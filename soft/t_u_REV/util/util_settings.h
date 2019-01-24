// Copyright (c) 2016 Patrick Dowling
//
// Author: Patrick Dowling (pld@gurkenkiste.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <stdint.h>
#include <string.h>
#include "../src/util_stream_buffer.h"
#include "../src/util_misc.h"

namespace settings {

enum StorageType {
  STORAGE_TYPE_U4, // nibbles are packed where possible, else aligned to next byte
  STORAGE_TYPE_I8, STORAGE_TYPE_U8,
  STORAGE_TYPE_I16, STORAGE_TYPE_U16,
  STORAGE_TYPE_I32, STORAGE_TYPE_U32,
  STORAGE_TYPE_NOP,
};

struct value_attr {
  const int default_;
  const int min_;
  const int max_;
  const char *name;
  const char * const *value_names;
  StorageType storage_type;

  // These can be used to introduce some dependent variables that are only
  // saved if parent_index > 0 and parent_value == get_value(parent_index - 1)
  // Obviously the parent has to live earlier in the settings list than the
  // dependent variable...
  int parent_index;
  int parent_value;

  int default_value() const {
    return default_;
  }

  int clamp(int value) const {
    if (value < min_) return min_;
    else if (value > max_) return max_;
    else return value;
  }
};

// Provide a very simple "settings" base.
// Settings values are an array of ints that are accessed by index, usually the
// owning class will use an enum for clarity, and provide specific getter
// functions for each value.
//
// The values are decsribed using the per-class value_attr array and allow min,
// max and default values. To set the defaults, call ::InitDefaults at least
// once before using the class. The min/max values are enforced when setting
// or modifying values. Classes shouldn't normally have to access the values_
// directly.
//
// To try and save some storage space, each setting can be stored as a smaller
// type as specified in the attributes. For even more compact representations,
// the owning class can pack things differently if required.
//
// TODO: Save/Restore is still kind of sucky
// TODO: If absolutely necessary, add STORAGE_TYPE_BIT and pack nibbles & bits
//
template <typename clazz, size_t num_settings>
class SettingsBase {
public:

  int get_value(size_t index) const {
    return values_[index];
  }

  static int clamp_value(size_t index, int value) {
    return value_attr_[index].clamp(value);
  }

  bool apply_value(size_t index, int value) {
    if (index < num_settings) {
      const int clamped = value_attr_[index].clamp(value);
      if (values_[index] != clamped) {
        values_[index] = clamped;
        return true;
      }
    }
    return false;
  }

  bool change_value(size_t index, int delta) {
    return apply_value(index, values_[index] + delta);
  }

  bool change_value_max(size_t index, int delta, int max) {
    if (index < num_settings) {
      int clamped = value_attr_[index].clamp(values_[index] + delta);
      if (clamped > max)
        clamped = max;
      if (values_[index] != clamped) {
        values_[index] = clamped;
        return true;
      }
    }
    return false;
  }

  static const settings::value_attr &value_attr(size_t i) {
    return value_attr_[i];
  }

  void InitDefaults() {
    for (size_t s = 0; s < num_settings; ++s)
      values_[s] = value_attr_[s].default_value();
  }

  size_t Save(util::StreamBufferWriter &stream_writer) const {
    nibbles_ = 0;
    for (size_t s = 0; s < num_settings; ++s) {
      auto attr = value_attr_[s];
      if (!check_parent_value(attr))
        continue;
      switch(attr.storage_type) {
        case STORAGE_TYPE_U4: write_nibble(stream_writer, s); break;
        case STORAGE_TYPE_I8: write_setting<int8_t>(stream_writer, s); break;
        case STORAGE_TYPE_U8: write_setting<uint8_t>(stream_writer, s); break;
        case STORAGE_TYPE_I16: write_setting<int16_t>(stream_writer, s); break;
        case STORAGE_TYPE_U16: write_setting<uint16_t>(stream_writer, s); break;
        case STORAGE_TYPE_I32: write_setting<int32_t>(stream_writer, s); break;
        case STORAGE_TYPE_U32: write_setting<uint32_t>(stream_writer, s); break;
        case STORAGE_TYPE_NOP: break;
      }
    }
    flush_nibbles(stream_writer);

    return stream_writer.overflow() ? 0 : stream_writer.written();
  }

  size_t Restore(util::StreamBufferReader &stream_reader) {
    nibbles_ = 0;
    for (size_t s = 0; s < num_settings; ++s) {
      auto attr = value_attr_[s];
      if (!check_parent_value(attr))
        continue;
      switch(attr.storage_type) {
        case STORAGE_TYPE_U4: read_nibble(stream_reader, s); break;
        case STORAGE_TYPE_I8: read_setting<int8_t>(stream_reader, s); break;
        case STORAGE_TYPE_U8: read_setting<uint8_t>(stream_reader, s); break;
        case STORAGE_TYPE_I16: read_setting<int16_t>(stream_reader, s); break;
        case STORAGE_TYPE_U16: read_setting<uint16_t>(stream_reader, s); break;
        case STORAGE_TYPE_I32: read_setting<int32_t>(stream_reader, s); break;
        case STORAGE_TYPE_U32: read_setting<uint32_t>(stream_reader, s); break;
        case STORAGE_TYPE_NOP: break;
      }
    }

    return stream_reader.underflow() ? 0 : stream_reader.underflow();
  }

  static size_t storageSize() {
    return storage_size_;
  }

protected:

  static constexpr uint16_t kNibbleValid = 0xf000;
  int values_[num_settings];
  static const settings::value_attr value_attr_[];
  static const size_t storage_size_;

  mutable uint16_t nibbles_;

  void flush_nibbles(util::StreamBufferWriter &stream_writer) const {
    if (nibbles_) {
      stream_writer.Write<uint8_t>(nibbles_ & 0xff);
      nibbles_ = 0;
    }
  }

  void write_nibble(util::StreamBufferWriter &stream_writer, size_t index) const {
    if (nibbles_) {
      nibbles_ |= (values_[index] & 0x0f);
      flush_nibbles(stream_writer);
    } else {
      // Ensure correct packing for reads even if there's an odd number of nibbles;
      // the first nibble is assumed to be in the msbits.
      nibbles_ = kNibbleValid | ((values_[index] & 0x0f) << 4);
    }
  }

  template <typename storage_type>
  void write_setting(util::StreamBufferWriter &stream_writer, size_t index) const {
    flush_nibbles(stream_writer);
    stream_writer.Write(static_cast<storage_type>(values_[index]));
  }

  void read_nibble(util::StreamBufferReader &stream_reader, size_t index) {
    uint8_t value;
    if (nibbles_) {
      value = nibbles_ & 0x0f;
      nibbles_ = 0;
    } else {
      value = stream_reader.Read<uint8_t>();
      nibbles_ = kNibbleValid | value;
      value >>= 4;
    }
    apply_value(index, value);
  }

  template <typename storage_type>
  void read_setting(util::StreamBufferReader &stream_reader, size_t index) {
    nibbles_ = 0;
    storage_type value = stream_reader.Read<storage_type>();
    apply_value(index, value);
  }

  static size_t calc_storage_size() {
    size_t s = 0;
    unsigned nibbles = 0;
    for (auto attr : value_attr_) {
      if (STORAGE_TYPE_U4 == attr.storage_type) {
        ++nibbles;
      } else {
        if (nibbles & 1) ++nibbles;
        switch(attr.storage_type) {
          case STORAGE_TYPE_I8: s += sizeof(int8_t); break;
          case STORAGE_TYPE_U8: s += sizeof(uint8_t); break;
          case STORAGE_TYPE_I16: s += sizeof(int16_t); break;
          case STORAGE_TYPE_U16: s += sizeof(uint16_t); break;
          case STORAGE_TYPE_I32: s += sizeof(int32_t); break;
          case STORAGE_TYPE_U32: s += sizeof(uint32_t); break;
          default: break;
        }
      }
    }
    if (nibbles & 1) ++nibbles;
    s += nibbles >> 1;
    return s;
  }

  bool check_parent_value(const settings::value_attr &attr) const {
    if (!attr.parent_index)
      return true;
    return attr.parent_value == get_value(attr.parent_index - 1);
  }
};

#define SETTINGS_DECLARE(clazz, last) \
template <> const size_t settings::SettingsBase<clazz, last>::storage_size_ = settings::SettingsBase<clazz, last>::calc_storage_size(); \
template <> const settings::value_attr settings::SettingsBase<clazz, last>::value_attr_[] =

#define VALID_IF(parent, value) \
parent + 1, static_cast<int>(value)

}; // namespace settings

#endif // SETTINGS_H_
