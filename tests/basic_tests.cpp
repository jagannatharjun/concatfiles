#include <algorithm>
#include <concatfiles.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include <vector>
auto file_list(gupta::cf_path base) {
  std::vector<std::pair<gupta::cf_path, int64_t>> l;
  for (auto f : std::filesystem::recursive_directory_iterator{base})
    if (!std::filesystem::is_directory(f))
      l.push_back(
          {std::filesystem::relative(f, base), std::filesystem::file_size(f)});
  return l;
}

#include <assert.h>
std::vector<uint8_t> readAll(gupta::cf_path f) {
  auto file = std::fopen(f.string().c_str(), "rb");
  assert(file != nullptr);
  std::vector<uint8_t> buf;
  uint8_t rbuf[10240];
  int64_t readsz = 0;
  while ((readsz = fread(rbuf, 1, sizeof rbuf, file))) {
    for (int i = 0; i < readsz; i++)
      buf.push_back(rbuf[i]);
  }
  return buf;
}

template <typename T> std::vector<uint8_t> readAll(T f) {
  uint8_t rbuf[10240];
  std::vector<uint8_t> buf;
  int64_t readsz = 0;
  while ((readsz = f->read(rbuf, sizeof rbuf))) {
    for (int i = 0; i < readsz; i++)
      buf.push_back(rbuf[i]);
  }
  return buf;
}

void dump(const char *f, const std::vector<uint8_t> &buf) {
  auto t = std::fopen(f, "wb");
  assert(t);
  fwrite(buf.data(), 1, buf.size(), t);
  fclose(t);
}

TEST(basicTest, zeroFileSize) {
  gupta::cf_path test_dir =
      R"(E:\Cpp\Projects\Obsolete\WebsiteBuilder-20180812T140735Z-001\WebsiteBuilder\Debug\Wbuilder.tlog)";
  if (std::filesystem::exists(test_dir / "zerosizefile"))
    std::filesystem::remove(test_dir / "zerosizefile");
  {
    auto f = std::fopen((test_dir / "zerosizefile").string().c_str(), "wb");
    fclose(f);
    ASSERT_EQ(std::filesystem::file_size(test_dir / "zerosizefile"), 0);
  }
  std::vector<uint8_t> buf;

  {
    auto cf = gupta::concatfiles(test_dir);
    buf = readAll(cf);
    puts("done reading");
  }

  {
    auto a = gupta::openConcatFileStream(buf.data(), buf.size());
    auto l = file_list(test_dir);
    int i = 0;
    for (auto f = a->next_file(); f; f = a->next_file(), i++) {
      auto p =
          std::find(l.begin(), l.end(),
                    std::pair<gupta::cf_path, int64_t>(f->path(), f->size()));
      ASSERT_NE(p, l.end()) << f->path() << " can't found,size - " << f->size();
      auto originalContent = readAll(test_dir / f->path());
      auto archiveContent = readAll(f.get());
      /*dump("E:\\1", originalContent);
      dump("E:\\2", archiveContent);*/
      ASSERT_EQ(originalContent.size(), archiveContent.size());
      ASSERT_EQ(originalContent, archiveContent);
    }
    ASSERT_EQ(l.size(), i) << " a doesn't gave back all files ";
  }
}

TEST(basicTest, basicTest) {
  gupta::cf_path test_dir =
      R"(E:\Cpp\Projects\Obsolete\WebsiteBuilder-20180812T140735Z-001\WebsiteBuilder\Debug\Wbuilder.tlog)";
  std::vector<uint8_t> buf;

  {
    auto cf = gupta::concatfiles(test_dir);
    buf = readAll(cf);
    puts("done reading");
  }

  {
    auto a = gupta::openConcatFileStream(buf.data(), buf.size());
    auto l = file_list(test_dir);
    int i = 0;
    for (auto f = a->next_file(); f; f = a->next_file(), i++) {
      auto p =
          std::find(l.begin(), l.end(),
                    std::pair<gupta::cf_path, int64_t>(f->path(), f->size()));
      ASSERT_NE(p, l.end()) << f->path() << " can't found,size - " << f->size();
      auto originalContent = readAll(test_dir / f->path());
      auto archiveContent = readAll(f.get());
      /*dump("E:\\1", originalContent);
      dump("E:\\2", archiveContent);*/
      ASSERT_EQ(originalContent.size(), archiveContent.size());
      ASSERT_EQ(originalContent, archiveContent);
    }
    ASSERT_EQ(l.size(), i) << " a doesn't gave back all files ";
  }
}

TEST(basicTest, bigdir) {
  gupta::cf_path test_dir =
      R"(E:\Cpp\Projects\Obsolete\WebsiteBuilder-20180812T140735Z-001\WebsiteBuilder)";
  std::vector<uint8_t> buf;

  {
    auto cf = gupta::concatfiles(test_dir);
    buf = readAll(cf);
    puts("done reading");
  }

  dump("E:\\f", buf);

  {
    auto a = gupta::openConcatFileStream(buf.data(), buf.size());
    auto l = file_list(test_dir);
    int i = 0;
    for (auto f = a->next_file(); f; f = a->next_file(), i++) {
      auto p =
          std::find(l.begin(), l.end(),
                    std::pair<gupta::cf_path, int64_t>(f->path(), f->size()));
      ASSERT_NE(p, l.end()) << f->path() << " can't found,size - " << f->size();
      auto originalContent = readAll(test_dir / f->path());
      auto archiveContent = readAll(f.get());
      /*dump("E:\\1", originalContent);
      dump("E:\\2", archiveContent);*/
      ASSERT_EQ(originalContent.size(), archiveContent.size());
      ASSERT_EQ(originalContent, archiveContent);
    }
    ASSERT_EQ(l.size(), i) << " a doesn't gave back all files ";
  }
}