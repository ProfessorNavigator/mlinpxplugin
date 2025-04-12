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
#ifndef COLLECTIONPROCESS_H
#define COLLECTIONPROCESS_H

#include <ArchEntry.h>
#include <AuxFunc.h>
#include <FileParseEntry.h>
#include <Hasher.h>
#include <functional>

#ifdef USE_OPENMP
#include <omp.h>
#endif
#ifndef USE_OPENMP
#include <atomic>
#include <condition_variable>
#include <mutex>
#endif

class CollectionProcess
{
public:
  CollectionProcess(const std::shared_ptr<AuxFunc> &af, const int &thr_num);

  virtual ~CollectionProcess();

  void
  collectFiles(const std::filesystem::path &inpx_path,
               const std::filesystem::path &books_path,
               const std::string &coll_name);

  void
  createBase();

  void
  stopAll();

  std::function<void(const double &current_sz, const double &total_sz)>
      signal_progress;

private:
  void
  parseInp(const std::filesystem::path &arch_path, const ArchEntry &e,
           FileParseEntry &fpe);

  void
  parseEntry(const std::string &ent, FileParseEntry &fpe);

  std::shared_ptr<AuxFunc> af;
  int thr_num = 1;
#ifndef USE_OPENMP
  std::atomic<bool> cancel;
#endif
#ifdef USE_OPENMP
  bool cancel = false;
#endif  
  Hasher *hsh;

  std::vector<ArchEntry> books_entries_list;

  std::vector<FileParseEntry> base;
#ifndef USE_OPENMP
  std::mutex base_mtx;
#endif
#ifdef USE_OPENMP
  omp_lock_t base_mtx;
#endif

  std::filesystem::path inpx_path;
  std::filesystem::path books_path;
  std::string coll_name;

  double total_size = 0.0;
#ifdef USE_OPENMP
  double parsed_bytes = 0.0;
#endif

#ifndef USE_OPENMP
  std::atomic<double> parsed_bytes;
  int run_thr = 0;
  std::mutex run_thr_mtx;
  std::condition_variable run_thr_var;
#endif
};

#endif // COLLECTIONPROCESS_H
