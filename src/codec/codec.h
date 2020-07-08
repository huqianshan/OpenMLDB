//
// codec.h
// Copyright (C) 2017 4paradigm.com
// Author wangtaize
// Date 2017-03-31
//

#ifndef SRC_CODEC_CODEC_H_
#define SRC_CODEC_CODEC_H_

#include <stdio.h>
#include <stdlib.h>

#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "base/endianconv.h"
#include "base/strings.h"
#include "proto/common.pb.h"

namespace rtidb {
namespace codec {

using ProjectList = ::google::protobuf::RepeatedField<uint32_t>;
using Schema =
    ::google::protobuf::RepeatedPtrField<::rtidb::common::ColumnDesc>;
static constexpr uint8_t VERSION_LENGTH = 2;
static constexpr uint8_t SIZE_LENGTH = 4;
static constexpr uint8_t HEADER_LENGTH = VERSION_LENGTH + SIZE_LENGTH;
static constexpr uint32_t UINT24_MAX = (1 << 24) - 1;

struct RowContext;
class RowBuilder;
class RowView;
class RowProject;

// TODO(wangtaize) share the row codec context
struct RowContext {};

class RowProject {
 public:
    RowProject(const Schema& schema, const ProjectList& plist);

    ~RowProject();

    bool Init();

    bool Project(const int8_t* row_ptr, uint32_t row_size, int8_t** out_ptr,
                 uint32_t* out_size);

 private:
    const Schema& schema_;
    const ProjectList& plist_;
    Schema output_schema_;
    // TODO(wangtaize) share the init overhead
    RowBuilder* row_builder_;
    RowView* row_view_;
};

class RowBuilder {
 public:
    explicit RowBuilder(const Schema& schema);

    uint32_t CalTotalLength(uint32_t string_length);
    bool SetBuffer(int8_t* buf, uint32_t size);
    bool SetBuffer(int8_t* buf, uint32_t size, bool need_clear);
    bool AppendBool(bool val);
    bool AppendInt32(int32_t val);
    bool AppendInt16(int16_t val);
    bool AppendInt64(int64_t val);
    bool AppendBlob(int64_t val);
    bool AppendTimestamp(int64_t val);
    bool AppendFloat(float val);
    bool AppendDouble(double val);
    bool AppendString(const char* val, uint32_t length);
    bool AppendNULL();
    bool AppendDate(uint32_t year, uint32_t month, uint32_t day);
    // append the date that encoded
    bool AppendDate(int32_t date);
    bool AppendValue(const std::string& val);
    bool SetBool(uint32_t index, bool val);
    bool SetInt32(uint32_t index, int32_t val);
    bool SetInt16(uint32_t index, int16_t val);
    bool SetInt64(uint32_t index, int64_t val);
    bool SetBlob(uint32_t index, int64_t val);
    bool SetTimestamp(uint32_t index, int64_t val);
    bool SetFloat(uint32_t index, float val);
    bool SetDouble(uint32_t index, double val);
    bool SetDate(uint32_t index, uint32_t year, uint32_t month, uint32_t day);
    // set the date that encoded
    bool SetDate(uint32_t index, int32_t date);

    void SetSchemaVersion(uint8_t version);

 private:
    bool Check(uint32_t index, ::rtidb::type::DataType type);
    inline void SetField(uint32_t index);
    inline void SetStrOffset(uint32_t str_pos);
    bool SetString(uint32_t index, const char* val, uint32_t length);
    bool SetNULL(uint32_t index);

 private:
    const Schema& schema_;
    int8_t* buf_;
    uint32_t cnt_;
    uint32_t size_;
    uint32_t str_field_cnt_;
    uint32_t str_addr_length_;
    uint32_t str_field_start_offset_;
    uint32_t str_offset_;
    uint8_t schema_version_;
    std::vector<uint32_t> offset_vec_;
};

class RowView {
 public:
    RowView(const Schema& schema, const int8_t* row, uint32_t size);
    explicit RowView(const Schema& schema);
    ~RowView() = default;
    bool Reset(const int8_t* row, uint32_t size);
    bool Reset(const int8_t* row);

    static uint8_t GetSchemaVersion(const int8_t* row) {
        return *(reinterpret_cast<const uint8_t*>(row + 1));
    }

    int32_t GetBool(uint32_t idx, bool* val);
    int32_t GetInt32(uint32_t idx, int32_t* val);
    int32_t GetInt64(uint32_t idx, int64_t* val);
    int32_t GetBlob(uint32_t idx, int64_t* val);
    int32_t GetTimestamp(uint32_t idx, int64_t* val);
    int32_t GetInt16(uint32_t idx, int16_t* val);
    int32_t GetFloat(uint32_t idx, float* val);
    int32_t GetDouble(uint32_t idx, double* val);
    int32_t GetString(uint32_t idx, char** val, uint32_t* length);
    int32_t GetDate(uint32_t idx, uint32_t* year, uint32_t* month,
                    uint32_t* day);
    int32_t GetDate(uint32_t idx, int32_t* date);
    bool IsNULL(uint32_t idx) { return IsNULL(row_, idx); }
    inline bool IsNULL(const int8_t* row, uint32_t idx) {
        const int8_t* ptr = row + HEADER_LENGTH + (idx >> 3);
        return *(reinterpret_cast<const uint8_t*>(ptr)) & (1 << (idx & 0x07));
    }
    inline uint32_t GetSize() { return size_; }

    static inline uint32_t GetSize(const int8_t* row) {
        return *(reinterpret_cast<const uint32_t*>(row + VERSION_LENGTH));
    }

    int32_t GetValue(const int8_t* row, uint32_t idx,
                     ::rtidb::type::DataType type, void* val);

    int32_t GetInteger(const int8_t* row, uint32_t idx,
                       ::rtidb::type::DataType type, int64_t* val);

    int32_t GetValue(const int8_t* row, uint32_t idx, char** val,
                     uint32_t* length);

    int32_t GetStrValue(const int8_t* row, uint32_t idx, std::string* val);
    int32_t GetStrValue(uint32_t idx, std::string* val);

 private:
    bool Init();
    bool CheckValid(uint32_t idx, ::rtidb::type::DataType type);

 private:
    uint8_t str_addr_length_;
    bool is_valid_;
    uint32_t string_field_cnt_;
    uint32_t str_field_start_offset_;
    uint32_t size_;
    const int8_t* row_;
    const Schema& schema_;
    std::vector<uint32_t> offset_vec_;
};

namespace v1 {

static constexpr uint8_t VERSION_LENGTH = 2;
static constexpr uint8_t SIZE_LENGTH = 4;
// calc the total row size with primary_size, str field count and str_size
inline uint32_t CalcTotalLength(uint32_t primary_size, uint32_t str_field_cnt,
                                uint32_t str_size, uint32_t* str_addr_space) {
    uint32_t total_size = primary_size + str_size;
    if (total_size + str_field_cnt <= UINT8_MAX) {
        *str_addr_space = 1;
        return total_size + str_field_cnt;
    } else if (total_size + str_field_cnt * 2 <= UINT16_MAX) {
        *str_addr_space = 2;
        return total_size + str_field_cnt * 2;
    } else if (total_size + str_field_cnt * 3 <= 1 << 24) {
        *str_addr_space = 3;
        return total_size + str_field_cnt * 3;
    } else {
        *str_addr_space = 4;
        return total_size + str_field_cnt * 4;
    }
}
inline int32_t AppendInt16(int8_t* buf_ptr, uint32_t buf_size, int16_t val,
                           uint32_t field_offset) {
    if (field_offset + 2 > buf_size) {
        return -1;
    }
    *(reinterpret_cast<int16_t*>(buf_ptr + field_offset)) = val;
    return 4;
}

inline int32_t AppendFloat(int8_t* buf_ptr, uint32_t buf_size, float val,
                           uint32_t field_offset) {
    if (field_offset + 4 > buf_size) {
        return -1;
    }
    *(reinterpret_cast<float*>(buf_ptr + field_offset)) = val;
    return 4;
}

inline int32_t AppendInt32(int8_t* buf_ptr, uint32_t buf_size, int32_t val,
                           uint32_t field_offset) {
    if (field_offset + 4 > buf_size) {
        return -1;
    }
    *(reinterpret_cast<int32_t*>(buf_ptr + field_offset)) = val;
    return 4;
}

inline int32_t AppendInt64(int8_t* buf_ptr, uint32_t buf_size, int64_t val,
                           uint32_t field_offset) {
    if (field_offset + 8 > buf_size) {
        return -1;
    }
    *(reinterpret_cast<int64_t*>(buf_ptr + field_offset)) = val;
    return 8;
}

inline int32_t AppendDouble(int8_t* buf_ptr, uint32_t buf_size, double val,
                            uint32_t field_offset) {
    if (field_offset + 8 > buf_size) {
        return -1;
    }

    *(reinterpret_cast<double*>(buf_ptr + field_offset)) = val;
    return 8;
}

int32_t AppendString(int8_t* buf_ptr, uint32_t buf_size, int8_t* val,
                     uint32_t size, uint32_t str_start_offset,
                     uint32_t str_field_offset, uint32_t str_addr_space,
                     uint32_t str_body_offset);

inline int8_t GetAddrSpace(uint32_t size) {
    if (size <= UINT8_MAX) {
        return 1;
    } else if (size <= UINT16_MAX) {
        return 2;
    } else if (size <= 1 << 24) {
        return 3;
    } else {
        return 4;
    }
}

inline int8_t GetBoolField(const int8_t* row, uint32_t offset) {
    int8_t value = *(row + offset);
    return value;
}

inline int16_t GetInt16Field(const int8_t* row, uint32_t offset) {
    return *(reinterpret_cast<const int16_t*>(row + offset));
}

inline int32_t GetInt32Field(const int8_t* row, uint32_t offset) {
    return *(reinterpret_cast<const int32_t*>(row + offset));
}

inline int64_t GetInt64Field(const int8_t* row, uint32_t offset) {
    return *(reinterpret_cast<const int64_t*>(row + offset));
}

inline float GetFloatField(const int8_t* row, uint32_t offset) {
    return *(reinterpret_cast<const float*>(row + offset));
}

inline double GetDoubleField(const int8_t* row, uint32_t offset) {
    return *(reinterpret_cast<const double*>(row + offset));
}

// native get string field method
int32_t GetStrField(const int8_t* row, uint32_t str_field_offset,
                    uint32_t next_str_field_offset, uint32_t str_start_offset,
                    uint32_t addr_space, int8_t** data, uint32_t* size);
int32_t GetCol(int8_t* input, int32_t offset, int32_t type_id, int8_t* data);
int32_t GetStrCol(int8_t* input, int32_t str_field_offset,
                  int32_t next_str_field_offset, int32_t str_start_offset,
                  int32_t type_id, int8_t* data);
}  // namespace v1

inline std::string Int64ToString(const int64_t key) {
    std::stringstream ss;
    ss << std::hex << key;
    std::string key_str = ss.str();
    return key_str;
}

}  // namespace codec
}  // namespace rtidb

#endif  // SRC_CODEC_CODEC_H_
