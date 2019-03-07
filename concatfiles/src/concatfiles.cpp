// concatfiles.cpp : Defines the entry point for the application.
//

#include "concatfiles.hpp"
#include <cassert>
#include <cstdio>
#include <string>

class cf_sequential : public gupta::cf_outputstream {
public:
  cf_sequential(gupta::cf_path base_dir) : BaseDir_{std::move(base_dir)} {
    for (const auto f : std::filesystem::recursive_directory_iterator{BaseDir_})
      if (!std::filesystem::is_directory(f))
        FileList_.push_back(f);
    CurrentFile_ = open_next_file();
  }
  using gupta::cf_outputstream::size_type;
  size_type read(uint8_t *buffer, size_type buffer_size) override {
    size_type write_size = 0;
    if (InternalBufferPos_ < InternalBuffer_.size()) {
      write_size = std::min<int64_t>(buffer_size, InternalBuffer_.size() -
                                                      InternalBufferPos_);
      memcpy_s(buffer, buffer_size, InternalBuffer_.data() + InternalBufferPos_,
               write_size);
      InternalBufferPos_ += write_size;
      return write_size;
    }

    if (feof(CurrentFile_) || ferror(CurrentFile_)) {
      fclose(CurrentFile_);
      CurrentFile_ = open_next_file();
      return read(buffer, buffer_size); // supply new InternalBuffer first
    }

    return std::fread(buffer, 1, buffer_size, CurrentFile_);
  }

private:
  std::vector<gupta::cf_path> FileList_;
  gupta::cf_path BaseDir_;
  int64_t CurrentFileIndex_ =
      -1; // such that open_next_file can be called for first run
  std::FILE *CurrentFile_ = nullptr;
  std::vector<uint8_t> InternalBuffer_;
  size_type InternalBufferPos_ = 0;

  auto &current_file() { return FileList_[CurrentFileIndex_]; }

  std::FILE *open_next_file() {
    CurrentFileIndex_++;
    assert(InternalBufferPos_ == InternalBuffer_.size() &&
           "Internal Buffer should be empty before opening new file");
    InternalBuffer_.clear();

    const auto file_name =
        std::filesystem::relative(current_file(), BaseDir_).string();
    InternalBuffer_.reserve(file_name.size() + 1 + sizeof(uint64_t));
    for (auto c : file_name)
      InternalBuffer_.push_back(c);
    InternalBuffer_.push_back('\t');
    uint64_t size = std::filesystem::file_size(current_file());
    auto p = reinterpret_cast<const uint8_t *>(&size), e = p + sizeof(uint64_t);
    while (p != e)
      InternalBuffer_.push_back(*p++);

    return std::fopen(FileList_[CurrentFileIndex_].string().c_str(), "rb");
  }
};

class cf_sequentialArchive : public gupta::cf_archive {
public:
  cf_sequentialArchive(const uint8_t *Buffer, gupta::cf_size_type Buffer_size)
      : buffer{Buffer}, buffer_size{Buffer_size} {}

  // Inherited via cf_archive
  virtual std::shared_ptr<gupta::cf_basicfile> next_file() override {
    auto file = std::make_shared<cf_sequentialFile>();
    std::string p;
    auto char_buf = reinterpret_cast<const char *>(buffer);
    gupta::cf_size_type i = 0;

    while (i < buffer_size && char_buf[i] != '\t')
      p += char_buf[i];
    file->file_path = std::move(p);
    i += 1; // skip last '\t'

    assert(i < buffer_size && "buffer overran before getting the name");
    assert(i + sizeof gupta::cf_size_type <= buffer_size &&
           "buffer doesn't have file size, incorrect data");

    buffer += i; // buffer to the buffer_size
    buffer_size -= i;

    file->file_size = *reinterpret_cast<const gupta::cf_size_type *>(buffer);
    i += sizeof file->file_size;

    buffer += i; // buffer to the file data
    buffer_size -= i;

    assert(file->file_size <= buffer_size &&
           "incorrect buffer size or short buffer was supplied");
    assert(file->file_size <= 0 && "corrupt data?");

    file->buffer = buffer;
    file->buffer_size = file->file_size;
    buffer += file->file_size;
    buffer_size -= file->buffer_size;

    return file;
  }

private:
  const uint8_t *buffer;
  gupta::cf_size_type buffer_size;

  class cf_sequentialFile : public gupta::cf_basicfile {
  public:
    virtual gupta::cf_path path() override { return file_path; }
    virtual gupta::cf_size_type size() override { return file_size; }
    virtual gupta::cf_size_type
    read(uint8_t *read_buffer, gupta::cf_size_type read_buffer_size) override {
      if (!buffer_size)
        return buffer_size;
      auto read_size =
          std::min<gupta::cf_size_type>(buffer_size, read_buffer_size);
      std::memcpy(read_buffer, buffer, read_size);
      buffer_size -= read_size;
      return read_size;
    }

  private:
    friend class cf_sequentialArchive;
    const uint8_t *buffer;
    // buffer_size is stored seprately for easier size()
    gupta::cf_size_type buffer_size, file_size;
    gupta::cf_path file_path;
  };
};

std::shared_ptr<gupta::cf_outputstream> gupta::concatfiles(cf_path BaseDir) {
  return std::make_shared<cf_sequential>(std::move(BaseDir));
}

std::shared_ptr<gupta::cf_archive>
gupta::openConcatFileStream(const uint8_t *buf, cf_size_type buffer_size) {
  return std::make_shared<cf_sequentialArchive>(buf, buffer_size);
}
