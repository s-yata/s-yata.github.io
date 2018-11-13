// Copyright 2010 Susumu Yata <syata@acm.org>

#include <bzlib.h>
#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#define ERROR(fmt, ...) \
  error_at_line(-(__LINE__), errno, __FILE__, __LINE__, fmt, ## __VA_ARGS__)

enum TestMode {
  COMPRESS_MODE,    // 圧縮モード（デフォルト）
  DECOMPRESS_MODE,  // 伸長モード
  HELP_MODE,        // ヘルプ表示
  DEFAULT_MODE = COMPRESS_MODE
};

enum DecompressorType {
  DEFAULT_DECOMPRESSOR,
  SMALL_DECOMPRESSOR
};

enum {
  DEFAULT_BLOCK_SIZE = 9
};

enum IOBufSize {
  INPUT_BUF_SIZE = 65536,  // 入力に使うバッファのサイズ
  OUTPUT_BUF_SIZE = 65536  // 出力に使うバッファのサイズ
};

enum TestMode test_mode = DEFAULT_MODE;
enum DecompressorType decompressor_type = DEFAULT_DECOMPRESSOR;
const char *output_file_name = NULL;
int block_size = DEFAULT_BLOCK_SIZE;
int work_factor = 0;
int verbosity = 0;

char input_buf[INPUT_BUF_SIZE];
char output_buf[OUTPUT_BUF_SIZE];

const char *ErrorMessage(int ret) {
  switch (ret) {
    case BZ_OK: {
      return "BZ_OK";
    }
    case BZ_RUN_OK: {
      return "BZ_RUN_OK";
    }
    case BZ_FLUSH_OK: {
      return "BZ_FLUSH_OK";
    }
    case BZ_FINISH_OK: {
      return "BZ_FINISH_OK";
    }
    case BZ_STREAM_END: {
      return "BZ_STREAM_END";
    }
    case BZ_SEQUENCE_ERROR: {
      return "BZ_SEQUENCE_ERROR";
    }
    case BZ_PARAM_ERROR: {
      return "BZ_PARAM_ERROR";
    }
    case BZ_MEM_ERROR: {
      return "BZ_MEM_ERROR";
    }
    case BZ_DATA_ERROR: {
      return "BZ_DATA_ERROR";
    }
    case BZ_DATA_ERROR_MAGIC: {
      return "BZ_DATA_ERROR_MAGIC";
    }
    case BZ_IO_ERROR: {
      return "BZ_IO_ERROR";
    }
    case BZ_UNEXPECTED_EOF: {
      return "BZ_UNEXPECTED_EOF";
    }
    case BZ_OUTBUFF_FULL: {
      return "BZ_OUTBUFF_FILL";
    }
    case BZ_CONFIG_ERROR: {
      return "BZ_CONFIG_ERROR";
    }
    default: {
      return "UNDEFINED_ERROR";
    }
  }
  return "";
}

int ParseInteger(const char *arg, long min_value, long max_value) {
  char *end_of_value;
  long value = strtol(optarg, &end_of_value, 10);
  if (*end_of_value != '\0' || (value < min_value) || (value > max_value)) {
    return -1;
  }
  return (int)value;
}

void ParseOptions(int argc, char *argv[]) {
  // getopt_long() で取得するオプションのリストです．
  // 2 番目のメンバが 1 のオプションは引数を取ります．
  static const struct option long_options[] = {
    { "compress", 0, NULL, 'c' },       // 圧縮モード
    { "decompress", 0, NULL, 'd' },     // 伸長モード
    { "blocksize100k", 1, NULL, 'b' },  // ブロックサイズ
    { "workfactor", 1, NULL, 'w' },     // 頑張り具合
    { "small", 0, NULL, 's' },          // メモリ節約
    { "output", 1, NULL, 'o' },         // 出力ファイル
    { "verbosity", 1, NULL, 'v' },      // 詳細表示
    { "help", 0, NULL, 'h' },           // ヘルプ表示
    { NULL, 0, NULL, '\0' }
  };

  // getopt_long() の第 3 引数はオプションのリストを受け取ります．
  // 引数を取るオプション文字には ':' が後続しています．
  int value;
  while ((value = getopt_long(argc, argv,
      "cdb:w:so:v:h", long_options, NULL)) != -1) {
    switch (value) {
      case 'c': {
        test_mode = COMPRESS_MODE;
        break;
      }
      case 'd': {
        test_mode = DECOMPRESS_MODE;
        break;
      }
      case 'b': {
        // ブロックサイズは 1 以上 9 以下です．
        block_size = ParseInteger(optarg, 1, 9);
        if (block_size < 0) {
          ERROR("invalid block size: %s", optarg);
        }
        break;
      }
      case 'w': {
        // 頑張り具合は 0 以上 250 以下です．
        work_factor = ParseInteger(optarg, 0, 250);
        if (work_factor < 0) {
          ERROR("invalid work factor: %s", optarg);
        }
        break;
      }
      case 's': {
        decompressor_type = SMALL_DECOMPRESSOR;
        break;
      }
      case 'v': {
        // 詳細表示のレベルは 0 以上 4 以下です．
        verbosity = ParseInteger(optarg, 0, 4);
        if (verbosity < 0) {
          ERROR("invalid verbosity level: %s", optarg);
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

void CompressInit(bz_stream *stream) {
  // メモリの確保・解放は libbzip2 に任せます．
  stream->bzalloc = NULL;
  stream->bzfree = NULL;
  stream->opaque = NULL;

  int ret = BZ2_bzCompressInit(stream, block_size, verbosity, work_factor);
  if (ret != BZ_OK) {
    ERROR("%s", ErrorMessage(ret));
  }
}

void CompressEnd(bz_stream *stream) {
  int ret = BZ2_bzCompressEnd(stream);
  if (ret != BZ_OK) {
    ERROR("%s", ErrorMessage(ret));
  }
}

void Compress(FILE *input_file, FILE *output_file) {
  bz_stream stream;
  CompressInit(&stream);
  int action = BZ_RUN;
  int ret = BZ_OK;
  do {
    stream.avail_in = fread(input_buf, 1, sizeof(input_buf), input_file);
    if (ferror(input_file) != 0) {
      ERROR("failed to read from file");
    } else if (feof(input_file) != 0) {
      // 入力が尽きれば BZ2_bzCompress() の第 2 引数を
      // BZ_FINISH に切り替えます．
      action = BZ_FINISH;
    }
    stream.next_in = input_buf;
    do {
      // 出力バッファを再設定して圧縮の続きをおこないます．
      stream.next_out = output_buf;
      stream.avail_out = sizeof(output_buf);
      ret = BZ2_bzCompress(&stream, action);
      if (ret == BZ_SEQUENCE_ERROR) {
        ERROR("%s", ErrorMessage(ret));
      }
      fwrite(output_buf, sizeof(output_buf) - stream.avail_out,
          1, output_file);
      if (ferror(output_file) != 0) {
        ERROR("failed to write into file");
      }
    } while ((stream.avail_out == 0) && (ret != BZ_STREAM_END));
    if (stream.avail_in != 0) {
      ERROR("unexpected bytes in input buffer");
    }
  } while (action != BZ_FINISH);
  if (ret != BZ_STREAM_END) {
    ERROR("failed to finish compress");
  }
  if (fflush(output_file) != 0) {
    ERROR("failed to flush output file");
  }
  CompressEnd(&stream);
}

void DecompressInit(bz_stream *stream) {
  // メモリの確保・解放は libbzip2 に任せます．
  stream->bzalloc = NULL;
  stream->bzfree = NULL;
  stream->opaque = NULL;

  int ret = BZ_OK;
  switch (decompressor_type) {
    case DEFAULT_DECOMPRESSOR: {
      ret = BZ2_bzDecompressInit(stream, verbosity, 0);
      break;
    }
    case SMALL_DECOMPRESSOR: {
      ret = BZ2_bzDecompressInit(stream, verbosity, 1);
      break;
    }
    default: {
      ERROR("invalid decompressor type: %d", decompressor_type);
      break;
    }
  }
  if (ret != BZ_OK) {
    ERROR("%s", ErrorMessage(ret));
  }
}

void DecompressEnd(bz_stream *stream) {
  int ret = BZ2_bzDecompressEnd(stream);
  if (ret != BZ_OK) {
    ERROR("%s", ErrorMessage(ret));
  }
}

void Decompress(FILE *input_file, FILE *output_file) {
  bz_stream stream;
  DecompressInit(&stream);
  int ret = BZ_OK;
  do {
    stream.avail_in = fread(input_buf, 1, sizeof(input_buf), input_file);
    if (ferror(input_file) != 0) {
      ERROR("failed to read from file");
    }
    if (stream.avail_in == 0) {
      // 入力が既に尽きている状態であれば，意図していないファイルの終端に
      // 到達したことになります．
      ERROR("unexpected end of file");
    }
    stream.next_in = input_buf;
    do {
      // 出力バッファを再設定して伸長の続きをおこないます．
      stream.next_out = output_buf;
      stream.avail_out = sizeof(output_buf);
      ret = BZ2_bzDecompress(&stream);
      if ((ret != BZ_OK) && (ret != BZ_STREAM_END)) {
        ERROR("%s", ErrorMessage(ret));
      }
      fwrite(output_buf, sizeof(output_buf) - stream.avail_out,
          1, output_file);
      if (ferror(output_file) != 0) {
        ERROR("failed to write into file");
      }
      // 一つのファイルに複数のデータが格納されているかもしれないので，
      // 返り値が BZ_STREAM_END であっても，入力が残っている状態であれば，
      // 内部状態をリセットして伸長を継続します．
      if ((ret == BZ_STREAM_END) &&
          ((stream.avail_in != 0) || (feof(input_file) == 0))) {
        DecompressEnd(&stream);
        DecompressInit(&stream);
        stream.avail_out = 0;
      }
    } while ((stream.avail_out == 0) && (ret != BZ_STREAM_END));
  } while (ret != BZ_STREAM_END);
  DecompressEnd(&stream);
}

void Code(FILE *input_file, FILE *output_file) {
  switch (test_mode) {
    case COMPRESS_MODE: {
      Compress(input_file, output_file);
      break;
    }
    case DECOMPRESS_MODE: {
      Decompress(input_file, output_file);
      break;
    }
    default: {
      ERROR("invalid mode: %d", test_mode);
    }
  }
}

int main(int argc, char *argv[]) {
  ParseOptions(argc, argv);
  if (test_mode == HELP_MODE) {
    printf("Usage: %s [OPTION]... [FILE]...\n"
        "Version: libbzip2-%s\n"
        "Options:\n"
        "  -c, --compress         圧縮します (default)\n"
        "  -d, --decompress       伸長します\n"
        "  -b, --blocksize100k=[1-9]  ブロックサイズを指定します (default: 9)\n"
        "  -w, --workfactor=[0-250]   代替アルゴリズムに切り替えるまでの\n"
        "                             頑張り具合を指定します (default: 30)\n"
        "  -s, --small            "
        "伸長時にメモリ消費の少ないアルゴリズムを使います\n"
        "  -o, --output=[FILE]    出力ファイルを指定します (default: stdout)\n"
        "  -v, --verbosity=[0-4]  "
        "詳細な情報を表示するようにします (default: 0)\n"
        "  -h, --help             このヘルプを表示します\n",
        argv[0], BZ2_bzlibVersion());
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
