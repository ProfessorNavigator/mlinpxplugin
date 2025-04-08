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
#ifndef COLLECTIONPROCESSGUI_H
#define COLLECTIONPROCESSGUI_H

#include <AuxFunc.h>
#include <CollectionProcess.h>
#include <glibmm-2.68/glibmm/dispatcher.h>
#include <gtkmm-4.0/gtkmm/label.h>
#include <gtkmm-4.0/gtkmm/progressbar.h>
#include <gtkmm-4.0/gtkmm/window.h>

class CollectionProcessGui
{
public:
  CollectionProcessGui(Gtk::Window *parent_window,
                       const std::shared_ptr<AuxFunc> &af, const int &thr_num);

  virtual ~CollectionProcessGui();

  void
  createWindow(const std::filesystem::path &inpx_path,
               const std::filesystem::path &books_path,
               const std::string &coll_name);

private:
  void
  launchProc(const std::filesystem::path &inpx_path,
             const std::filesystem::path &books_path,
             const std::string &coll_name);

  void
  completeMessage();

  Gtk::Window *parent_window;
  std::shared_ptr<AuxFunc> af;
  int thr_num;

  Gtk::ProgressBar *progress;

  CollectionProcess *coll_proc = nullptr;

  bool canceled = false;

  std::atomic<double> parsed_bytes;
  std::atomic<double> total_size;

  Glib::Dispatcher *progress_disp = nullptr;
  Glib::Dispatcher *ops_completed_disp = nullptr;

  Gtk::Window *main_window;
};

#endif // COLLECTIONPROCESSGUI_H
