//------------------------------------------------------------------------------
// Copyright (c) 2016 by contributors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//------------------------------------------------------------------------------

/*
Author: Chao Ma (mctt90@gmail.com)

This file is the implementation of Parser.
*/

#include "src/reader/parser.h"

#include "src/base/split_string.h"

#include <stdlib.h>

namespace xLearn {

//------------------------------------------------------------------------------
// Class register
//------------------------------------------------------------------------------
CLASS_REGISTER_IMPLEMENT_REGISTRY(xLearn_parser_registry, Parser);
REGISTER_PARSER("libsvm", LibsvmParser);
REGISTER_PARSER("libffm", FFMParser);
REGISTER_PARSER("csv", CSVParser);

// How many lines are there in current memory buffer
index_t Parser::get_line_number(char* buf, uint64 buf_size) {
  index_t num = 0;
  for (uint64 i = 0; i < buf_size; ++i) {
    if (buf[i] == '\n') num++;
  }
  // The last line may doesn't contain the '\n'
  // and we need +1 here
  if (buf[buf_size-1] == '\n') {
    return num;
  }
  return num + 1;
}

// Get one line from memory buffer
uint64 Parser::get_line_from_buffer(char* line,
                               char* buf,
                               uint64 pos,
                               uint64 size) {
  if (pos >= size) { return 0; }
  uint64 end_pos = pos;
  while (end_pos < size && buf[end_pos] != '\n') { end_pos++; }
  uint64 read_size = end_pos - pos + 1;
  if (read_size > kMaxLineSize) {
    LOG(FATAL) << "Encountered a too-long line.    \
                   Please check the data.";
  }
  memcpy(line, buf+pos, read_size);
  line[read_size - 1] = '\0';
  if (read_size > 1 && line[read_size - 2] == '\r') {
    // Handle some txt format in windows or DOS.
    line[read_size - 2] = '\0';
  }
  return read_size;
}

//------------------------------------------------------------------------------
// LibsvmParser parses the following data format:
// [y1 idx:value idx:value ...]
// [y2 idx:value idx:value ...]
//------------------------------------------------------------------------------
void LibsvmParser::Parse(char* buf, uint64 size, DMatrix& matrix) {
  CHECK_NOTNULL(buf);
  CHECK_GT(size, 0);
  index_t line_num = get_line_number(buf, size);
  matrix.ResetMatrix(line_num);
  static char* line_buf = new char[kMaxLineSize];
  // Parse every line
  uint64 pos = 0;
  for (index_t i = 0; i < line_num; ++i) {
    pos += get_line_from_buffer(line_buf, buf, pos, size);
    matrix.row[i] = new SparseRow();
    // Add Y
    char *y_char = strtok(line_buf, " \t");
    matrix.Y[i] = atof(y_char);
    // Add bias
    matrix.AddNode(i, 0, 1.0);
    // Add other features
    for (;;) {
      char *idx_char = strtok(nullptr, ":");
      char *value_char = strtok(nullptr, " \t");
      if(idx_char == nullptr || *idx_char == '\n') {
        break;
      }
      matrix.AddNode(i,
        atoi(idx_char),
        atof(value_char));
    }
  }
}

//------------------------------------------------------------------------------
// FFMParser parses the following data format:
// [y1 field:idx:value field:idx:value ...]
// [y2 field:idx:value field:idx:value ...]
//------------------------------------------------------------------------------
void FFMParser::Parse(char* buf, uint64 size, DMatrix& matrix) {
  CHECK_NOTNULL(buf);
  CHECK_GT(size, 0);
  index_t line_num = get_line_number(buf, size);
  matrix.ResetMatrix(line_num);
  static char* line_buf = new char[kMaxLineSize];
  // Parse every line
  uint64 pos = 0;
  for (index_t i = 0; i < line_num; ++i) {
    pos += get_line_from_buffer(line_buf, buf, pos, size);
    matrix.row[i] = new SparseRow();
    // Add Y
    char *y_char = strtok(line_buf, " \t");
    matrix.Y[i] = atof(y_char);
    // Add bias
    matrix.AddNode(i, 0, 1.0, 0);
    // Add other features
    for (;;) {
      char *field_char = strtok(nullptr, ":");
      char *idx_char = strtok(nullptr, ":");
      char *value_char = strtok(nullptr, " \t");
      if(field_char == nullptr || *field_char == '\n') {
        break;
      }
      index_t idx = atoi(idx_char);
      real_t value = atof(value_char);
      index_t field_id = atoi(field_char);
      matrix.AddNode(i, idx, value, field_id);
    }
  }
}

//------------------------------------------------------------------------------
// CSVParser parses the following data format:
// [feat_1 feat_2 feat_3 ... feat_n y1]
// [feat_1 feat_2 feat_3 ... feat_n y2]
//------------------------------------------------------------------------------
void CSVParser::Parse(char* buf, uint64 size, DMatrix& matrix) {
  CHECK_NOTNULL(buf);
  CHECK_GT(size, 0);
  index_t line_num = get_line_number(buf, size);
  matrix.ResetMatrix(line_num);
  static char* line_buf = new char[kMaxLineSize];
  // Parse every line
  uint64 pos = 0;
  std::vector<std::string> str_vec;
  for (index_t i = 0; i < line_num; ++i) {
    pos += get_line_from_buffer(line_buf, buf, pos, size);
    matrix.row[i] = new SparseRow();
    str_vec.clear();
    SplitStringUsing(line_buf, " \t", &str_vec);
    int size = str_vec.size();
    // Add Y
    matrix.Y[i] = atof(str_vec[size-1].c_str());
    // Add bias
    matrix.AddNode(i, 0, 1.0, 0);
    // Add other features
    for (int j = 0; j < size-1; ++j) {
      index_t idx = j+1;
      real_t value = atof(str_vec[j].c_str());
      // skip zero
      if (value < kVerySmallNumber) { continue; }
      matrix.AddNode(i, idx, value);
    }
  }
}

} // namespace xLearn
