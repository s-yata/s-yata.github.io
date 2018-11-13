// Copyright 2010 Susumu Yata <syata@acm.org>

#include <errno.h>
#include <error.h>
#include <lzma.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <getopt.h>

#define ERROR(fmt, ...) \
  error_at_line(-(__LINE__), errno, __FILE__, __LINE__, fmt, ## __VA_ARGS__)

enum TestMode {
  ENCODER_MODE,  // 圧縮モード（デフォルト）
  DECODER_MODE,  // 伸長モード
  HELP_MODE,     // ヘルプ表示
  DEFAULT_MODE = ENCODER_MODE
};

enum EncoderPreset {
  DEFAULT_PRESET = 6  // デフォルトの圧縮レベル
};

enum IOBufSize {
  INPUT_BUF_SIZE = 65536,  // 入力に使うバッファのサイズ
  OUTPUT_BUF_SIZE = 65536  // 出力に使うバッファのサイズ
};

enum TestMode test_mode = DEFAULT_MODE;
const char *output_file_name = NULL;
uint32_t encoder_preset = 6;
lzma_check check_format = LZMA_CHECK_CRC64;

uint8_t input_buf[INPUT_BUF_SIZE];
uint8_t output_buf[OUTPUT_BUF_SIZE];

const char *ErrorMessage(lzma_ret ret) {
  return "";
}

void ParseOptions(int argc, char *argv[]) {
  // getopt_long() で取得するオプションのリストです．
  // 2 番目のメンバが 1 のオプションは引数を取ります．
  static const struct option long_options[] = {
    { "encoder", 0, NULL, 'e' },  // 圧縮モード
    { "decoder", 0, NULL, 'd' },  // 伸長モード
    { "preset", 1, NULL, 'p' },   // 圧縮レベルとフラグ
    { "check", 1, NULL, 'c' },    // 誤り検出符号
    { "output", 1, NULL, 'o' },   // 出力ファイル
    { "help", 0, NULL, 'h' },     // ヘルプ表示
    { NULL, 0, NULL, '\0' }
  };

  // getopt_long() の第 3 引数はオプションのリストを受け取ります．
  // 引数を取るオプション文字には ':' が後続しています．
  int value;
  while ((value = getopt_long(argc, argv,
      "edp:c:o:h", long_options, NULL)) != -1) {
    switch (value) {
      case 'e': {
        test_mode = ENCODER_MODE;
        break;
      }
      case 'd': {
        test_mode = DECODER_MODE;
        break;
      }
      case 'p': {
        // 圧縮レベルは 0 以上 9 以下です．
        char *end_of_value;
        long value = strtol(optarg, &end_of_value, 10);
        if ((value < 0) || (value > 9)) {
          ERROR("invalid compression level: %s", optarg);
        }
        encoder_preset = (uint32_t)value;

        // 時間をかけて圧縮率を向上させるオプションがあります．
        if (*end_of_value == 'e') {
          encoder_preset |= LZMA_PRESET_EXTREME;
          ++end_of_value;
        }
        if (*end_of_value != '\0') {
          ERROR("invalid compression level: %s", optarg);
        }
        break;
      }
      case 'c': {
        // 誤り検出符号は <lzma/container.h> にて定義されている 4 種類です．
        if (strcasecmp(optarg, "NONE") == 0) {
          check_format = LZMA_CHECK_NONE;
        } else if (strcasecmp(optarg, "CRC32") == 0) {
          check_format = LZMA_CHECK_CRC32;
        } else if (strcasecmp(optarg, "CRC64") == 0) {
          check_format = LZMA_CHECK_CRC64;
        } else if (strcasecmp(optarg, "SHA256") == 0) {
          check_format = LZMA_CHECK_SHA256;
        } else {
          ERROR("invalid check format: %s", optarg);
        }
        break;
      }
      case 'o': {
        // 出力ファイルの名前を設定します．
        // 指定がなければ標準出力を使います．
        output_file_name = optarg;
        break;
      }
      case 'h': {
        test_mode = HELP_MODE;
        break;
      }
      default: {
        ERROR("invalid option");
      }
    }
  }
}

// XZ Utils では，圧縮・伸長のために呼び出す関数が共通なので，
// 特別な処理が必要でなければ，同じ関数でどちらにも対応することができます．
void CodeData(lzma_stream *stream, FILE *input_file, FILE *output_file) {
  lzma_action action = LZMA_RUN;
  lzma_ret ret = LZMA_OK;
  do {
    stream->avail_in = fread(input_buf, 1, sizeof(input_buf), input_file);
    if (ferror(input_file) != 0) {
      ERROR("failed to read from file");
    } else if (feof(input_file) != 0) {
      // 入力が尽きれば lzma_code() の第 2 引数を LZMA_FINISH に切り替えます．
      action = LZMA_FINISH;
    }
    stream->next_in = input_buf;
    do {
      // 出力バッファを再設定して圧縮・伸長の続きをおこないます．
      stream->next_out = output_buf;
      stream->avail_out = sizeof(output_buf);
      ret = lzma_code(stream, action);
      if ((ret != LZMA_OK) && (ret != LZMA_STREAM_END)) {
        ERROR("%s", ErrorMessage(ret));
      }
      fwrite(output_buf, sizeof(output_buf) - stream->avail_out,
          1, output_file);
      if (ferror(output_file) != 0) {
        ERROR("failed to write into file");
      }
    } while ((stream->avail_out == 0) && (ret != LZMA_STREAM_END));
    if (stream->avail_in != 0) {
      ERROR("unexpected bytes in input buffer");
    }
  } while (action != LZMA_FINISH);
  if (ret != LZMA_STREAM_END) {
    ERROR("failed to finish encode");
  }
  if (fflush(output_file) != 0) {
    ERROR("failed to flush output file");
  }
}

void Code(FILE *input_file, FILE *output_file) {
  // メモリの確保・解放は XZ Utils に任せます．
  lzma_stream stream = LZMA_STREAM_INIT;
  switch (test_mode) {
    case ENCODER_MODE: {
      lzma_ret ret = lzma_easy_encoder(
          &stream, encoder_preset, check_format);
      if (ret != LZMA_OK) {
        ERROR("%s", ErrorMessage(ret));
      }
      break;
    }
    case DECODER_MODE: {
      // 一つのファイルに複数のデータが格納されているかもしれないので，
      // LZMA_CONCATENATED を指定しておき，lzma_code() のループを単純化します．
      lzma_ret ret = lzma_stream_decoder(
          &stream, 128 << 20, LZMA_CONCATENATED);
      if (ret != LZMA_OK) {
        ERROR("%s", ErrorMessage(ret));
      }
      break;
    }
    default: {
      ERROR("invalid mode: %d", test_mode);
    }
  }
  CodeData(&stream, input_file, output_file);
  lzma_end(&stream);
}

int main(int argc, char *argv[]) {
  ParseOptions(argc, argv);
  if (test_mode == HELP_MODE) {
    printf("Usage: %s [OPTION]... [FILE]...\n"
        "Version: xz-utils-%s\n"
        "Options:\n"
        "  -e, --encoder  圧縮します (default)\n"
        "  -d, --decoder  伸長します\n"
        "  -p, --preset=[0-9][e]  "
        "圧縮レベルとフラグを指定します (default: 6)\n"
        "  -c, --check=[NONE, CRC32, CRC64, SHA256]\n"
        "                         誤り検出符号を指定します (default: CRC64)\n"
        "  -o, --output=[FILE]    出力ファイルを指定します (default: stdout)\n"
        "  -h, --help     このヘルプを表示します\n",
        argv[0], lzma_version_string());
    return 0;
  }

  FILE *output_file = stdout;
  if (output_file_name != NULL) {
    output_file = fopen(output_file_name, "wb");
    if (output_file == NULL) {
      ERROR("%s", output_file_name);
    }
  }

  // 入力ファイルの指定がなければ，標準入力を使います．
  if (optind >= argc) {
    Code(stdin, output_file);
  } else {
    for (int i = optind; i < argc; ++i) {
      const char *input_file_name = argv[i];
      FILE *input_file = fopen(input_file_name, "rb");
      if (input_file == NULL) {
        ERROR("%s", input_file_name);
      }

      Code(input_file, output_file);
      if (fclose(input_file) != 0) {
        ERROR("%s", input_file_name);
      }
    }
  }

  if (output_file != stdout) {
    if (fclose(output_file) != 0) {
      ERROR("%s", output_file_name);
    }
  }

  return 0;
}
