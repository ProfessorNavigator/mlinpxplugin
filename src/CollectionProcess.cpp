/*
 * Copyright (C) 2025 Yury Bobylev <bobilev_yury@mail.ru>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include <ByteOrder.h>
#include <CollectionProcess.h>
#include <SelfRemovingPath.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>

#ifndef USE_OPENMP
#include <thread>
#endif

CollectionProcess::CollectionProcess(const std::shared_ptr<AuxFunc> &af,
                                     const int &thr_num)
{
  this->af = af;
  this->thr_num = thr_num;
  la = new LibArchive(af);
  hsh = new Hasher(af);
#ifndef USE_OPENMP
  cancel.store(false);
  parsed_bytes.store(0.0);
#endif
#ifdef USE_OPENMP
  omp_init_lock(&base_mtx);
#endif
}

CollectionProcess::~CollectionProcess()
{
  delete la;
  delete hsh;
#ifdef USE_OPENMP
  omp_destroy_lock(&base_mtx);
#endif
}

void
CollectionProcess::collectFiles(const std::filesystem::path &inpx_path,
                                const std::filesystem::path &books_path,
                                const std::string &coll_name)
{
  this->books_path = books_path;
  this->coll_name = coll_name;
  this->inpx_path = inpx_path;
  la->fileNames(inpx_path, books_entries_list);

  for(auto it = books_entries_list.begin(); it != books_entries_list.end();)
    {
#ifndef USE_OPENMP
      if(cancel.load())
        {
          break;
        }
#endif
#ifdef USE_OPENMP
      bool cncl;
#pragma omp atomic read
      cncl = cancel;
      if(cncl)
        {
          break;
        }
#endif
      std::filesystem::path p = std::filesystem::u8path(it->filename);
      if(p.extension().u8string() == ".inp")
        {
          std::filesystem::path bfp;
          std::error_code ec;
          for(auto &pp : std::filesystem::directory_iterator(books_path, ec))
            {
              if(pp.path().stem() == p.stem())
                {
                  bfp = pp;
                  break;
                }
            }
          if(ec)
            {
              std::cout << "CollectionProcess::collectFiles " << ec.message()
                        << std::endl;
              books_entries_list.clear();
              break;
            }
          if(bfp.empty())
            {
              books_entries_list.erase(it);
              continue;
            }
          uintmax_t sz = std::filesystem::file_size(bfp, ec);
          if(ec)
            {
              std::cout << "CollectionProcess::collectFiles " << bfp << " "
                        << ec.message() << std::endl;
              books_entries_list.erase(it);
            }
          else
            {
              total_size += static_cast<double>(sz);
              it++;
            }
        }
      else
        {
          it++;
        }
    }
}

void
CollectionProcess::createBase()
{
#ifndef USE_OPENMP
  for(auto it = books_entries_list.begin(); it != books_entries_list.end();
      it++)
    {
      if(cancel.load())
        {
          break;
        }
      ArchEntry ent = *it;
      std::unique_lock<std::mutex> ullock(run_thr_mtx);
      run_thr++;
      std::thread thr([this, ent] {
        FileParseEntry fpe;
        std::filesystem::path p = std::filesystem::u8path(ent.filename);
        std::error_code ec;
        std::filesystem::path found_p;
        for(auto &pp : std::filesystem::directory_iterator(books_path, ec))
          {
            if(pp.path().stem() == p.stem())
              {
                found_p = pp.path();
                break;
              }
          }
        if(ec)
          {
            std::cout << "CollectionProcess::createBase error: "
                      << ec.message() << std::endl;
            cancel.store(true);
            std::lock_guard<std::mutex> lglock(run_thr_mtx);
            run_thr--;
            run_thr_var.notify_all();
            return void();
          }
        if(found_p.empty())
          {
            std::lock_guard<std::mutex> lglock(run_thr_mtx);
            run_thr--;
            run_thr_var.notify_all();
            return void();
          }
        fpe.file_rel_path = found_p.filename().u8string();
        p = found_p;

        double sz = static_cast<double>(std::filesystem::file_size(p, ec));
        if(ec)
          {
            std::cout << "CollectionProcess::createBase error: "
                      << ec.message() << std::endl;
          }
        else
          {
            std::thread thr([this, p, &fpe] {
              fpe.file_hash = hsh->file_hashing(p);
            });

            parseInp(inpx_path, ent, fpe);

            thr.join();

            base_mtx.lock();
            base.emplace_back(fpe);
            base_mtx.unlock();
          }

        parsed_bytes.store(parsed_bytes.load() + sz);
        if(signal_progress)
          {
            signal_progress(parsed_bytes.load(), total_size);
          }
        std::lock_guard<std::mutex> lglock(run_thr_mtx);
        run_thr--;
        run_thr_var.notify_all();
      });
      thr.detach();

      run_thr_var.wait(ullock, [this] {
        return run_thr < thr_num;
      });
    }

  std::unique_lock<std::mutex> ullock(run_thr_mtx);
  run_thr_var.wait(ullock, [this] {
    return run_thr == 0;
  });
#endif

#ifdef USE_OPENMP
  omp_set_num_threads(thr_num);
  omp_set_dynamic(true);
  omp_set_max_active_levels(1);
#pragma omp parallel
#pragma omp for
  for(auto it = books_entries_list.begin(); it != books_entries_list.end();
      it++)
    {
      bool cncl;
#pragma omp atomic read
      cncl = cancel;
      if(cncl)
        {
#pragma omp cancel for
          continue;
        }
      ArchEntry ent = *it;
      FileParseEntry fpe;
      std::filesystem::path p = std::filesystem::u8path(ent.filename);
      std::error_code ec;
      std::filesystem::path found_p;
      for(auto &pp : std::filesystem::directory_iterator(books_path, ec))
        {
          if(pp.path().stem() == p.stem())
            {
              found_p = pp.path();
              break;
            }
        }
      if(ec)
        {
          std::cout << "CollectionProcess::createBase error: " << ec.message()
                    << std::endl;
#pragma omp atomic write
          cancel = true;
#pragma omp cancel for
          continue;
        }
      if(found_p.empty())
        {
          continue;
        }
      fpe.file_rel_path = found_p.filename().u8string();
      p = found_p;

      double sz = static_cast<double>(std::filesystem::file_size(p, ec));
      if(ec)
        {
          std::cout << "CollectionProcess::createBase error: " << ec.message()
                    << std::endl;
        }
      else
        {
#pragma omp parallel
#pragma omp master
          {
#pragma omp masked
            {
              omp_event_handle_t event;
#pragma omp task detach(event)
              {
                fpe.file_hash = hsh->file_hashing(p);
                omp_fulfill_event(event);
              }
            }

            parseInp(inpx_path, ent, fpe);
#pragma omp taskwait
          }

          omp_set_lock(&base_mtx);
          base.emplace_back(fpe);
          omp_unset_lock(&base_mtx);
        }

#pragma omp atomic capture
      {
        parsed_bytes += sz;
        sz = parsed_bytes;
      }
      if(signal_progress)
        {
          signal_progress(sz, total_size);
        }
    }
#endif

  std::filesystem::path base_path = af->homePath();
  base_path /= std::filesystem::u8path(".local/share/MyLibrary/Collections");
  base_path /= std::filesystem::u8path(coll_name);
  base_path /= std::filesystem::u8path("base");
  std::filesystem::create_directories(base_path.parent_path());
  std::fstream f;
  f.open(base_path, std::ios_base::out | std::ios_base::binary);
  if(f.is_open())
    {
      std::string vl = books_path.u8string();
      uint16_t val16 = static_cast<uint16_t>(vl.size());
      ByteOrder bo = val16;
      bo.get_little(val16);
      size_t sz_16 = sizeof(val16);

      f.write(reinterpret_cast<char *>(&val16), sz_16);
      f.write(vl.c_str(), vl.size());

      size_t sz;
      uint64_t val64;
      size_t sz_64 = sizeof(val64);
      for(auto it = base.begin(); it != base.end(); it++)
        {
          std::string entry;

          val16 = static_cast<uint16_t>(it->file_rel_path.size());
          bo = val16;
          bo.get_little(val16);

          entry.resize(sz_16);
          std::memcpy(&entry[0], &val16, sz_16);

          entry += it->file_rel_path;

          val16 = static_cast<uint16_t>(it->file_hash.size());
          bo = val16;
          bo.get_little(val16);
          sz = entry.size();
          entry.resize(sz + sz_16);
          std::memcpy(&entry[sz], &val16, sz_16);

          entry += it->file_hash;

          for(auto it_b = it->books.begin(); it_b != it->books.end(); it_b++)
            {
              std::string book_entry;
              for(int i = 1; i <= 6; i++)
                {
                  switch(i)
                    {
                    case 1:
                      {
                        val16 = static_cast<uint16_t>(it_b->book_path.size());
                        break;
                      }
                    case 2:
                      {
                        val16
                            = static_cast<uint16_t>(it_b->book_author.size());
                        break;
                      }
                    case 3:
                      {
                        val16 = static_cast<uint16_t>(it_b->book_name.size());
                        break;
                      }
                    case 4:
                      {
                        val16
                            = static_cast<uint16_t>(it_b->book_series.size());
                        break;
                      }
                    case 5:
                      {
                        val16 = static_cast<uint16_t>(it_b->book_genre.size());
                        break;
                      }
                    case 6:
                      {
                        val16 = static_cast<uint16_t>(it_b->book_date.size());
                        break;
                      }
                    default:
                      {
                        break;
                      }
                    }
                  bo = val16;
                  bo.get_little(val16);
                  sz = book_entry.size();
                  book_entry.resize(sz + sz_16);
                  std::memcpy(&book_entry[sz], &val16, sz_16);

                  switch(i)
                    {
                    case 1:
                      {
                        book_entry += it_b->book_path;
                        break;
                      }
                    case 2:
                      {
                        book_entry += it_b->book_author;
                        break;
                      }
                    case 3:
                      {
                        book_entry += it_b->book_name;
                        break;
                      }
                    case 4:
                      {
                        book_entry += it_b->book_series;
                        break;
                      }
                    case 5:
                      {
                        book_entry += it_b->book_genre;
                        break;
                      }
                    case 6:
                      {
                        book_entry += it_b->book_date;
                        break;
                      }
                    default:
                      break;
                    }
                }

              val64 = static_cast<uint64_t>(book_entry.size());
              bo = val64;
              bo.get_little(val64);
              sz = entry.size();
              entry.resize(sz + sz_64);
              std::memcpy(&entry[sz], &val64, sz_64);

              entry += book_entry;
            }
          val64 = static_cast<uint64_t>(entry.size());
          bo = val64;
          bo.get_little(val64);

          f.write(reinterpret_cast<char *>(&val64), sz_64);
          f.write(entry.c_str(), entry.size());
        }

      f.close();
    }
}

void
CollectionProcess::stopAll()
{
#ifndef USE_OPENMP
  cancel.store(true);
#endif
#ifdef USE_OPENMP
#pragma omp atomic write
  cancel = true;
#endif
  if(hsh)
    {
      hsh->cancelAll();
    }
}

void
CollectionProcess::parseInp(const std::filesystem::path &arch_path,
                            const ArchEntry &e, FileParseEntry &fpe)
{
  std::filesystem::path p
      = af->temp_path() / std::filesystem::u8path(af->randomFileName());
  SelfRemovingPath srp(p);

  std::filesystem::path inp_p = la->unpackByPosition(arch_path, srp.path, e);

  std::fstream f;
  f.open(inp_p, std::ios_base::in | std::ios_base::binary);
  if(f.is_open())
    {
      std::string fl_str;
      f.seekg(0, std::ios_base::end);
      fl_str.resize(f.tellg());
      f.seekg(0, std::ios_base::beg);
      f.read(fl_str.data(), fl_str.size());
      f.close();

      std::string find_str = { 0x0d, 0x0a };
      std::string::size_type n_beg = 0;
      std::string::size_type n_end = 0;
      for(;;)
        {
#ifndef USE_OPENMP
          if(cancel.load())
            {
              break;
            }
#endif
#ifdef USE_OPENMP
          bool cncl;
#pragma omp atomic read
          cncl = cancel;
          if(cncl)
            {
              break;
            }
#endif
          n_end = fl_str.find(find_str, n_beg);
          if(n_end != std::string::npos)
            {
              std::string ent(fl_str.begin() + n_beg, fl_str.begin() + n_end);
              parseEntry(ent, fpe);
            }
          else
            {
              break;
            }
          n_beg = n_end + find_str.size();
          if(n_beg >= fl_str.size())
            {
              break;
            }
        }
    }
}

void
CollectionProcess::parseEntry(const std::string &ent, FileParseEntry &fpe)
{
  BookParseEntry bpe;
  std::string::size_type n_beg = 0;
  std::string::size_type n_end = 0;
  std::string find_str = { 0x04 };
  for(int i = 1; i <= 11; i++)
    {
      n_end = ent.find(find_str, n_beg);
      if(n_end != std::string::npos)
        {
          switch(i)
            {
            case 1:
              {
                bpe.book_author
                    = std::string(ent.begin() + n_beg, ent.begin() + n_end);
                bpe.book_author.erase(std::remove_if(bpe.book_author.begin(),
                                                     bpe.book_author.end(),
                                                     [](char &el) {
                                                       if(el >= 0 && el <= 32)
                                                         {
                                                           return true;
                                                         }
                                                       else
                                                         {
                                                           return false;
                                                         }
                                                     }),
                                      bpe.book_author.end());
                if(bpe.book_author.size() > 0)
                  {
                    if(*bpe.book_author.rbegin() == ':')
                      {
                        bpe.book_author.pop_back();
                      }
                  }

                std::string insert = ", ";
                for(auto it = bpe.book_author.begin();
                    it != bpe.book_author.end();)
                  {
                    switch(*it)
                      {
                      case ',':
                        {
                          *it = ' ';
                          it++;
                          break;
                        }
                      case ':':
                        {
                          it = bpe.book_author.erase(it);
                          it = bpe.book_author.insert(it, insert.begin(),
                                                      insert.end());
                          it++;
                          it++;
                          break;
                        }
                      default:
                        {
                          it++;
                          break;
                        }
                      }
                  }
                std::string::size_type n = 0;
                std::string f_str = " ,";
                while(bpe.book_author.size() > 0)
                  {
                    n = bpe.book_author.find(f_str, n);
                    if(n != std::string::npos)
                      {
                        bpe.book_author.erase(bpe.book_author.begin() + n);
                      }
                    else
                      {
                        break;
                      }
                  }

                while(bpe.book_author.size() > 0)
                  {
                    char ch = *bpe.book_author.begin();
                    if(ch >= 0 && ch <= 32)
                      {
                        bpe.book_author.erase(bpe.book_author.begin());
                      }
                    else
                      {
                        break;
                      }
                  }
                while(bpe.book_author.size() > 0)
                  {
                    char ch = *bpe.book_author.rbegin();
                    if(ch >= 0 && ch <= 32)
                      {
                        bpe.book_author.pop_back();
                      }
                    else
                      {
                        break;
                      }
                  }
                break;
              }
            case 2:
              {
                bpe.book_genre
                    = std::string(ent.begin() + n_beg, ent.begin() + n_end);
                bpe.book_genre.erase(std::remove_if(bpe.book_genre.begin(),
                                                    bpe.book_genre.end(),
                                                    [](char &el) {
                                                      if(el >= 0 && el <= 32)
                                                        {
                                                          return true;
                                                        }
                                                      else
                                                        {
                                                          return false;
                                                        }
                                                    }),
                                     bpe.book_genre.end());
                if(bpe.book_genre.size() > 0)
                  {
                    if(*bpe.book_genre.rbegin() == ':')
                      {
                        bpe.book_genre.pop_back();
                      }
                  }

                std::string insert = ", ";
                for(auto it = bpe.book_genre.begin();
                    it != bpe.book_genre.end();)
                  {
                    switch(*it)
                      {
                      case ',':
                        {
                          *it = ' ';
                          it++;
                          break;
                        }
                      case ':':
                        {
                          it = bpe.book_genre.erase(it);
                          it = bpe.book_genre.insert(it, insert.begin(),
                                                     insert.end());
                          it++;
                          it++;
                          break;
                        }
                      default:
                        {
                          it++;
                          break;
                        }
                      }
                  }
                std::string::size_type n = 0;
                std::string f_str = " ,";
                while(bpe.book_genre.size() > 0)
                  {
                    n = bpe.book_genre.find(f_str, n);
                    if(n != std::string::npos)
                      {
                        bpe.book_genre.erase(bpe.book_genre.begin() + n);
                      }
                    else
                      {
                        break;
                      }
                  }
                break;
              }
            case 3:
              {
                bpe.book_name
                    = std::string(ent.begin() + n_beg, ent.begin() + n_end);
                break;
              }
            case 4:
              {
                bpe.book_series
                    = std::string(ent.begin() + n_beg, ent.begin() + n_end);
                break;
              }
            case 5:
              {
                std::string s_nm(ent.begin() + n_beg, ent.begin() + n_end);
                if(!s_nm.empty())
                  {
                    bpe.book_series += " " + s_nm;
                  }
                break;
              }
            case 6:
              {
                bpe.book_path
                    = std::string(ent.begin() + n_beg, ent.begin() + n_end);
                break;
              }
            case 10:
              {
                std::string ext(ent.begin() + n_beg, ent.begin() + n_end);
                if(!ext.empty())
                  {
                    bpe.book_path += "." + ext;
                  }
                break;
              }
            case 11:
              {
                bpe.book_date
                    = std::string(ent.begin() + n_beg, ent.begin() + n_end);
                break;
              }
            default:
              break;
            }
        }
      else
        {
          break;
        }
      n_beg = n_end + find_str.size();
      if(n_beg >= ent.size())
        {
          break;
        }
    }
  fpe.books.emplace_back(bpe);
}
