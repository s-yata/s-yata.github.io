// Copyright 2012 Susumu Yata <syata@acm.org>

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <limits.h>
#include <snappy-c.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define ERROR(fmt, ...) \
  error_at_line(-(__LINE__), errno, __FILE__, __LINE__, fmt, ## __VA_ARGS__)

enum TestMode {
  COMPRESS_MODE,    // 圧縮モード（デフォルト）
  UNCOMPRESS_MODE,  // 伸長モード
  HELP_MODE,        // ヘルプ表示
  DEFAULT_MODE = COMPRESS_MODE
};

// ファイル全体を単一のブロックとして圧縮することも可能ですが，
// 大きなファイルを圧縮するときに十分なメモリを確保できないと困るので，
// 小さなブロックに分割してから圧縮するようにしました．
enum CompressionBlockSize {
  MIN_BLOCK_SIZE     = 10,  // 最小のブロックサイズ
  MAX_BLOCK_SIZE     = 30,  // 最大のブロックサイズ
  DEFAULT_BLOCK_SIZE = 20   // デフォルトのブロックサイズ
};

enum TestMode test_mode = DEFAULT_MODE;
size_t block_size = 1U << DEFAULT_BLOCK_SIZE;
const char *output_file_name = NULL;

void ParseOptions(int argc, char *argv[]) {
  // getopt_long() で取得するオプションのリストです．
  // 2 番目のメンバが 1 のオプションは引数を取ります．
  static const struct option long_options[] = {
    { "compress", 0, NULL, 'c' },    // 圧縮モード
    { "uncompress", 0, NULL, 'u' },  // 伸長モード
    { "block-size", 0, NULL, 'b' },  // ブロックサイズ
    { "output", 1, NULL, 'o' },      // 出力ファイル
    { "help", 0, NULL, 'h' },        // ヘルプ表示
    { NULL, 0, NULL, '\0' }
  };

  // getopt_long() の第 3 引数はオプションのリストを受け取ります．
  // 引数を取るオプション文字には ':' が後続しています．
  int value;
  while ((value = getopt_long(argc, argv,
                              "cub:o:h", long_options, NULL)) != -1) {
    switch (value) {
      case 'c': {
        test_mode = COMPRESS_MODE;
        break;
      }
      case 'u': {
        test_mode = UNCOMPRESS_MODE;
        break;
      }
      case 'b': {
        char *end_of_value;
        long value = strtol(optarg, &end_of_value, 10);
        if ((*end_of_value != '\0') ||
            (value < MIN_BLOCK_SIZE) || (value > MAX_BLOCK_SIZE)) {
          ERROR("invalid compression level: %s", optarg);
        }
        block_size = 1 << value;
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

void Compress(FILE *input_file, FILE *output_file) {
  // 最初に必要となるメモリを確保しておきます．
  char * const block = (char *)malloc(block_size);
  if (block == NULL) {
    ERROR("failed to allocate memory to input buffer: %zu bytes",
          block_size);
  }
  const size_t output_buf_size = snappy_max_compressed_length(block_size);
  char * const output_buf = (char *)malloc(output_buf_size);
  if (output_buf == NULL) {
    ERROR("failed to allocate memory to output buffer: %zu bytes",
          output_buf_size);
  }

  for ( ; ; ) {
    // ブロック単位で読み込みます．
    const size_t input_size = fread(block, 1, block_size, input_file);
    if (input_size != block_size) {
      if (ferror(input_file) != 0) {
        ERROR("failed to read data from file");
      } else if (input_size == 0) {
        break;
      }
    }

    // 最初にバッファの確保を済ませているため，
    // snappy_compress() を呼び出すだけで圧縮できます．
    size_t output_size = output_buf_size;
    const snappy_status status = 
        snappy_compress(block, input_size, output_buf, &output_size);
    if (status != SNAPPY_OK) {
      ERROR("buffer is too small: %zu bytes", output_buf_size);
    }

    // 伸長するときに圧縮データのサイズが必要となるため，
    // ヘッダとしてサイズを出力してから圧縮データを出力します．
    const uint32_t header = (uint32_t)output_size;
    if (fwrite(&header, sizeof(header), 1, output_file) != 1) {
      ERROR("failed to write header into file");
    }
    if (fwrite(output_buf, output_size, 1, output_file) != 1) {
      ERROR("failed to write data into file: %zu bytes", output_size);
    }
  }

  free(block);
  free(output_buf);
}

void Uncompress(FILE *input_file, FILE *output_file) {
  char *input_buf = NULL;
  char *output_buf = NULL;
  size_t input_buf_size = 0;
  size_t output_buf_size = 0;

  for ( ; ; ) {
    // 圧縮データのサイズを読み込み，必要に応じてバッファを拡張した後で，
    // 圧縮データの読み込みをおこないます．
    uint32_t header;
    size_t input_size = fread(&header, 1, sizeof(header), input_file);
    if (input_size != sizeof(header)) {
      if (ferror(input_file) != 0) {
        ERROR("failed to read header from file");
      } else if (input_size == 0) {
        break;
      }
      ERROR("invalid format: incomplete header");
    }

    input_size = header;
    if (input_buf_size < input_size) {
      char * const new_buf = (char *)realloc(input_buf, input_size);
      if (new_buf == NULL) {
        ERROR("failed to allocate memory to input buffer: %zu bytes",
              input_size);
      }
      input_buf = new_buf;
      input_buf_size = input_size;
    }

    if (fread(input_buf, input_size, 1, input_file) != 1) {
      ERROR("failed to read data from file: %zu bytes", input_size);
    }

    // 伸長後のサイズを求め，必要に応じてバッファを拡張した後で，
    // 圧縮データの伸長をおこないます．
    size_t output_size;
    snappy_status status =
        snappy_uncompressed_length(input_buf, input_size, &output_size);
    if (status != SNAPPY_OK) {
      ERROR("unexpected bytes in input buffer");
    }

    if (output_buf_size < output_size) {
      char * const new_buf = (char *)realloc(output_buf, output_size);
      if (new_buf == NULL) {
        ERROR("failed to allocate memory to output buffer: %zu bytes",
              output_size);
      }
      output_buf = new_buf;
      output_buf_size = output_size;
    }

    status = snappy_uncompress(input_buf, input_size, output_buf, &output_size);
    if (status == SNAPPY_BUFFER_TOO_SMALL) {
      ERROR("output buffer is too small: %zu bytes", output_size);
    } else if (status == SNAPPY_INVALID_INPUT) {
      ERROR("input file is in invalid format");
    }

    // 伸長したデータを出力します．
    if (fwrite(output_buf, output_size, 1, output_file) != 1) {
      ERROR("failed to write data into file: %zu bytes", output_size);
    }
  }

  free(input_buf);
  free(output_buf);
}

void Code(FILE *input_file, FILE *output_file) {
  switch (test_mode) {
    case COMPRESS_MODE: {
      Compress(input_file, output_file);
      break;
    }
    case UNCOMPRESS_MODE: {
      Uncompress(input_file, output_file);
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
           "Options:\n"
           "  -c, --compress       圧縮します (default)\n"
           "  -u, --uncompress     伸長します\n"
           "  -b, --block-size     圧縮時のブロックサイズを指定します"
           " (default: 20)\n"
           "  -o, --output=[FILE]  出力ファイルを指定します (default: stdout)\n"
           "  -h, --help           このヘルプを表示します\n",
           argv[0]);
    return 0;
  }

  // 出力ファイルの指定がなければ，標準出力を使います．
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
