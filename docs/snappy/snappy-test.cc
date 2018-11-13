// Copyright 2012 Susumu Yata <syata@acm.org>

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <limits.h>
#include <snappy.h>
#include <stdio.h>
#include <stdlib.h>

#include <exception>
#include <fstream>
#include <iostream>
#include <vector>

#define ERROR(fmt, ...) \
  ::error_at_line(-(__LINE__), errno, __FILE__, __LINE__, fmt, ## __VA_ARGS__)

namespace {

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

void Compress(std::istream *input_stream, std::ostream *output_stream) try {
  std::vector<char> input_buf;
  std::string output_buf;

  for ( ; ; ) {
    // ブロック単位で読み込みます．
    input_buf.resize(block_size);
    input_stream->read(&*input_buf.begin(), input_buf.size());
    if (input_stream->fail() && !input_stream->eof()) {
      ERROR("failed to read data from file");
    }
    input_buf.resize(input_stream->gcount());
    if (input_buf.empty()) {
      break;
    }

    // snappy::Compress() を呼び出すだけで圧縮できます．
    const uint32_t output_size = static_cast<uint32_t>(
        snappy::Compress(&*input_buf.begin(), input_buf.size(), &output_buf));

    // 伸長するときに圧縮データのサイズが必要となるため，
    // ヘッダとしてサイズを出力してから圧縮データを出力します．
    if (!output_stream->write(reinterpret_cast<const char *>(&output_size),
                              sizeof(output_size))) {
      ERROR("failed to write header into file");
    }
    if (!output_stream->write(&*output_buf.begin(), output_size)) {
      ERROR("failed to write data into file: %u bytes", output_size);
    }
  }
} catch (const std::exception &ex) {
  ERROR("%s", ex.what());
}

void Uncompress(std::istream *input_stream, std::ostream *output_stream) try {
  std::vector<char> input_buf;
  std::string output_buf;

  for ( ; ; ) {
    // 圧縮データのサイズを読み込み，バッファのサイズを調整した後で，
    // 圧縮データの読み込みをおこないます．
    uint32_t input_size;
    input_stream->read(reinterpret_cast<char *>(&input_size),
                       sizeof(input_size));
    if (input_stream->fail() && !input_stream->eof()) {
      ERROR("failed to read header from file");
    } else if (input_stream->gcount() != sizeof(input_size)) {
      if (input_stream->gcount() == 0) {
        break;
      }
      ERROR("invalid format: incomplete header");
    }
    input_buf.resize(input_size);
    if (!input_stream->read(&*input_buf.begin(), input_buf.size())) {
      ERROR("failed to read data from file: %zu bytes", input_buf.size());
    }

    // snappy::Uncompress() を呼び出すだけで伸長できます．
    if (!snappy::Uncompress(&*input_buf.begin(), input_buf.size(),
                            &output_buf)) {
      ERROR("failed to uncompress data");
    }

    // 伸長したデータを出力します．
    if (!output_stream->write(&*output_buf.begin(), output_buf.size())) {
      ERROR("failed to write data into file: %zu bytes", output_buf.size());
    }
  }
} catch (const std::exception &ex) {
  ERROR("%s", ex.what());
}

void Code(std::istream *input_stream, std::ostream *output_stream) {
  switch (test_mode) {
    case COMPRESS_MODE: {
      Compress(input_stream, output_stream);
      break;
    }
    case UNCOMPRESS_MODE: {
      Uncompress(input_stream, output_stream);
      break;
    }
    default: {
      ERROR("invalid mode: %d", test_mode);
    }
  }
}

}  // namespace

int main(int argc, char *argv[]) try {
  ParseOptions(argc, argv);
  if (test_mode == HELP_MODE) {
    std::cerr << "Usage: " << argv[0] << " [OPTION]... [FILE]...\n"
                 "Version: snappy-" << SNAPPY_MAJOR << '.'
                                    << SNAPPY_MINOR << '.'
                                    << SNAPPY_PATCHLEVEL << "\n"
                 "Options:\n"
                 "  -c, --compress       圧縮します (default)\n"
                 "  -u, --uncompress     伸長します\n"
                 "  -b, --block-size     圧縮時のブロックサイズを指定します"
                 " (default: 20)\n"
                 "  -o, --output=[FILE]  "
                 "出力ファイルを指定します (default: stdout)\n"
                 "  -h, --help           このヘルプを表示します" << std::endl;
    return 0;
  }

  // 出力ファイルの指定がなければ，標準出力を使います．
  std::ofstream output_file;
  std::ostream *output_stream = &std::cout;
  if (output_file_name != NULL) {
    output_file.open(output_file_name, std::ios::binary);
    if (!output_file) {
      ERROR("%s", output_file_name);
    }
    output_stream = &output_file;
  }

  // 入力ファイルの指定がなければ，標準入力を使います．
  if (optind >= argc) {
    Code(&std::cin, output_stream);
  } else {
    for (int i = optind; i < argc; ++i) {
      const char * const input_file_name = argv[i];
      std::ifstream input_file(input_file_name, std::ios::binary);
      if (!input_file) {
        ERROR("%s", input_file_name);
      }
      Code(&input_file, output_stream);
    }
  }

  return 0;
} catch (const std::exception &ex) {
  ERROR("%s", ex.what());
}
