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
#ifndef MLINPXPLUGIN_H
#define MLINPXPLUGIN_H

#include <MLPlugin.h>
#include <gtkmm-4.0/gtkmm/entry.h>

#ifndef ML_GTK_OLD
#include <gtkmm-4.0/gtkmm/filedialog.h>
#endif
#ifdef ML_GTK_OLD
#include <gtkmm-4.0/gtkmm/filechooserdialog.h>
#endif

class MLInpxPlugin : public MLPlugin
{
public:
  MLInpxPlugin(void *af_ptr);

  void
  createWindow(Gtk::Window *parent_window) override;

protected:
  void
  setWindowSizes();

  void
  fileDialog(const int &variant);

#ifndef ML_GTK_OLD
  void
  inpxFilePathSlot(const Glib::RefPtr<Gio::AsyncResult> &result,
                   const Glib::RefPtr<Gtk::FileDialog> &fd);

  void
  booksDirectoryPathSlot(const Glib::RefPtr<Gio::AsyncResult> &result,
                         const Glib::RefPtr<Gtk::FileDialog> &fd);
#endif

#ifdef ML_GTK_OLD
  void
  inpxFilePathSlot(int respons_id, Gtk::FileChooserDialog *fd);

  void
  booksDirectoryPathSlot(int respons_id, Gtk::FileChooserDialog *fd);
#endif

  void
  checkEntries();

  void
  confirmationDialog(const std::filesystem::path &inpx_path,
                     const std::filesystem::path &books_path,
                     const std::string &coll_name, const int &variant);

  Gtk::Window *parent_window;
  Gtk::Window *main_window;
  Gtk::Entry *path_to_inpx;
  Gtk::Entry *path_to_books;
  Gtk::Entry *collection_name;
  Gtk::Entry *thr_num;
};

extern "C"
{
#ifdef __linux
  MLPlugin *
  create(void *af_ptr)
  {
    return new MLInpxPlugin(af_ptr);
  }
#endif
#ifdef _WIN32
  __declspec(dllexport) MLPlugin *
  create(void *af_ptr)
  {
    return new MLInpxPlugin(af_ptr);
  }
#endif
}

#endif // MLINPXPLUGIN_H
