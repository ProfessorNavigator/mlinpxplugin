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
#include <CollectionProcessGui.h>
#include <MLInpxPlugin.h>
#include <giomm-2.68/giomm/liststore.h>
#include <gtkmm-4.0/gdkmm/monitor.h>
#include <gtkmm-4.0/gtkmm/box.h>
#include <gtkmm-4.0/gtkmm/button.h>
#include <gtkmm-4.0/gtkmm/grid.h>
#include <gtkmm-4.0/gtkmm/label.h>
#include <iostream>
#include <libintl.h>
#include <sstream>

#ifdef USE_OPENMP
#include <omp.h>
#endif
#ifndef USE_OPENMP
#include <thread>
#endif

#ifndef ML_GTK_OLD
#include <gtkmm-4.0/gtkmm/error.h>
#endif

MLInpxPlugin::MLInpxPlugin(void *af_ptr) : MLPlugin(af_ptr)
{
  plugin_name = "MLInpxPlugin";
  std::filesystem::path txt_domain = af->share_path();
  txt_domain /= std::filesystem::u8path("locale");
  bindtextdomain(plugin_name.c_str(), txt_domain.u8string().c_str());
  bind_textdomain_codeset(plugin_name.c_str(), "UTF-8");
  textdomain(plugin_name.c_str());
  plugin_description
      = gettext("Plugin to import collections from .inpx files");
}

void
MLInpxPlugin::createWindow(Gtk::Window *parent_window)
{
#ifdef USE_OPENMP
#pragma omp parallel
#pragma omp masked
  {
#endif
    this->parent_window = parent_window;

    textdomain(plugin_name.c_str());

    main_window = new Gtk::Window;
    main_window->set_application(parent_window->get_application());
    main_window->set_title(plugin_name);
    main_window->set_name("MLwindow");
    main_window->set_transient_for(*parent_window);
    main_window->set_modal(true);
    setWindowSizes();

    Gtk::Grid *grid = Gtk::make_managed<Gtk::Grid>();
    grid->set_halign(Gtk::Align::FILL);
    grid->set_valign(Gtk::Align::FILL);
    main_window->set_child(*grid);

    Gtk::Label *lab = Gtk::make_managed<Gtk::Label>();
    lab->set_margin(5);
    lab->set_halign(Gtk::Align::START);
    lab->set_name("windowLabel");
    lab->set_text(gettext("Path to .inpx file:"));
    grid->attach(*lab, 0, 0, 2, 1);

    path_to_inpx = Gtk::make_managed<Gtk::Entry>();
    path_to_inpx->set_margin(5);
    path_to_inpx->set_halign(Gtk::Align::FILL);
    path_to_inpx->set_hexpand(true);
    path_to_inpx->set_name("windowEntry");
    grid->attach(*path_to_inpx, 0, 1, 1, 1);

    Gtk::Button *open = Gtk::make_managed<Gtk::Button>();
    open->set_margin(5);
    open->set_halign(Gtk::Align::CENTER);
    open->set_name("operationBut");
    open->set_label(gettext("Open"));
    open->signal_clicked().connect(
        std::bind(&MLInpxPlugin::fileDialog, this, 1));
    grid->attach(*open, 1, 1, 1, 1);

    lab = Gtk::make_managed<Gtk::Label>();
    lab->set_margin(5);
    lab->set_halign(Gtk::Align::START);
    lab->set_name("windowLabel");
    lab->set_text(gettext("Path to books directory:"));
    grid->attach(*lab, 0, 2, 2, 1);

    path_to_books = Gtk::make_managed<Gtk::Entry>();
    path_to_books->set_margin(5);
    path_to_books->set_halign(Gtk::Align::FILL);
    path_to_books->set_hexpand(true);
    path_to_books->set_name("windowEntry");
    grid->attach(*path_to_books, 0, 3, 1, 1);

    open = Gtk::make_managed<Gtk::Button>();
    open->set_margin(5);
    open->set_halign(Gtk::Align::CENTER);
    open->set_name("operationBut");
    open->set_label(gettext("Open"));
    open->signal_clicked().connect(
        std::bind(&MLInpxPlugin::fileDialog, this, 2));
    grid->attach(*open, 1, 3, 1, 1);

    lab = Gtk::make_managed<Gtk::Label>();
    lab->set_margin(5);
    lab->set_halign(Gtk::Align::START);
    lab->set_name("windowLabel");
    lab->set_text(gettext("New collection name:"));
    grid->attach(*lab, 0, 4, 2, 1);

    collection_name = Gtk::make_managed<Gtk::Entry>();
    collection_name->set_margin(5);
    collection_name->set_halign(Gtk::Align::FILL);
    collection_name->set_hexpand(true);
    collection_name->set_name("windowEntry");
    grid->attach(*collection_name, 0, 5, 2, 1);

    Gtk::Box *thr_box
        = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    grid->attach(*thr_box, 0, 6, 2, 1);

    lab = Gtk::make_managed<Gtk::Label>();
    lab->set_margin(5);
    lab->set_halign(Gtk::Align::START);
    lab->set_name("windowLabel");
    std::stringstream strm;
    strm.imbue(std::locale("C"));
#ifndef USE_OPENMP
    strm << std::thread::hardware_concurrency();
#endif
#ifdef USE_OPENMP
    strm << omp_get_num_procs();
#endif
    lab->set_text(Glib::ustring(gettext("Threads number")) + " ("
                  + gettext("recommended max number is") + " "
                  + Glib::ustring(strm.str()) + "):");
    thr_box->append(*lab);

    thr_num = Gtk::make_managed<Gtk::Entry>();
    thr_num->set_margin(5);
    thr_num->set_halign(Gtk::Align::START);
    thr_num->set_max_width_chars(3);
    thr_num->set_name("windowEntry");
    thr_num->set_alignment(Gtk::Align::CENTER);
    thr_num->set_text("1");
    thr_box->append(*thr_num);

    Gtk::Grid *controls_grid = Gtk::make_managed<Gtk::Grid>();
    controls_grid->set_halign(Gtk::Align::FILL);
    controls_grid->set_hexpand(true);
    controls_grid->set_column_homogeneous(true);
    grid->attach(*controls_grid, 0, 7, 2, 1);

    Gtk::Button *import = Gtk::make_managed<Gtk::Button>();
    import->set_margin(5);
    import->set_halign(Gtk::Align::CENTER);
    import->set_name("applyBut");
    import->set_label(gettext("Import"));
    import->signal_clicked().connect(
        std::bind(&MLInpxPlugin::checkEntries, this));
    controls_grid->attach(*import, 0, 0, 1, 1);

    Gtk::Button *cancel = Gtk::make_managed<Gtk::Button>();
    cancel->set_margin(5);
    cancel->set_halign(Gtk::Align::CENTER);
    cancel->set_name("cancelBut");
    cancel->set_label(gettext("Close"));
    cancel->signal_clicked().connect(
        std::bind(&Gtk::Window::close, main_window));
    controls_grid->attach(*cancel, 1, 0, 1, 1);

    main_window->signal_close_request().connect(
        [this] {
          std::unique_ptr<Gtk::Window> win(main_window);
          win->set_visible(false);
          main_window = nullptr;
          return true;
        },
        false);

    main_window->present();
#ifdef USE_OPENMP
  }
#endif
}

void
MLInpxPlugin::setWindowSizes()
{
  Glib::RefPtr<Gdk::Surface> surf = parent_window->get_surface();
  Glib::RefPtr<Gdk::Display> disp = parent_window->get_display();
  Glib::RefPtr<Gdk::Monitor> mon = disp->get_monitor_at_surface(surf);
  Gdk::Rectangle req;
  mon->get_geometry(req);

  req.set_width(req.get_width() * mon->get_scale_factor());
  req.set_height(req.get_height() * mon->get_scale_factor());

  int width = static_cast<int>(req.get_width());
  main_window->set_default_size(width * 0.5, -1);
}

void
MLInpxPlugin::fileDialog(const int &variant)
{
#ifndef ML_GTK_OLD
  Glib::RefPtr<Gtk::FileDialog> fd = Gtk::FileDialog::create();
  fd->set_modal(true);

  Glib::RefPtr<Gio::File> fl
      = Gio::File::create_for_path(af->homePath().u8string());
  fd->set_initial_folder(fl);

  Glib::RefPtr<Gio::Cancellable> cncl = Gio::Cancellable::create();
  if(variant == 1)
    {
      Glib::RefPtr<Gtk::FileFilter> filter = Gtk::FileFilter::create();
      filter->add_suffix("inpx");

      Glib::RefPtr<Gio::ListStore<Gtk::FileFilter>> list
          = Gio::ListStore<Gtk::FileFilter>::create();
      list->append(filter);

      fd->set_filters(list);

      fd->open(*main_window,
               std::bind(&MLInpxPlugin::inpxFilePathSlot, this,
                         std::placeholders::_1, fd),
               cncl);
    }
  else
    {
      fd->select_folder(*main_window,
                        std::bind(&MLInpxPlugin::booksDirectoryPathSlot, this,
                                  std::placeholders::_1, fd),
                        cncl);
    }
#endif

#ifdef ML_GTK_OLD
  Gtk::FileChooserDialog *fd;
  if(variant == 1)
    {
      fd = new Gtk::FileChooserDialog(*main_window, gettext("Open .inpx"),
                                      Gtk::FileChooser::Action::OPEN, true);
    }
  else
    {
      fd = new Gtk::FileChooserDialog(*main_window, gettext("Books directory"),
                                      Gtk::FileChooser::Action::SELECT_FOLDER,
                                      true);
    }
  fd->set_application(main_window->get_application());
  fd->set_modal(true);
  fd->set_name("MLwindow");

  Gtk::Button *but
      = fd->add_button(gettext("Cancel"), Gtk::ResponseType::CANCEL);
  but->set_margin(5);
  but->set_name("cancelBut");

  but = fd->add_button(gettext("Open"), Gtk::ResponseType::ACCEPT);
  but->set_margin(5);
  but->set_name("applyBut");

  Glib::RefPtr<Gio::File> initial
      = Gio::File::create_for_path(af->homePath().u8string());
  fd->set_current_folder(initial);

  if(variant == 1)
    {
      Glib::RefPtr<Gtk::FileFilter> filter = Gtk::FileFilter::create();
      filter->add_suffix("inpx");
      fd->set_filter(filter);

      fd->signal_response().connect(std::bind(
          &MLInpxPlugin::inpxFilePathSlot, this, std::placeholders::_1, fd));
    }
  else
    {
      fd->signal_response().connect(
          std::bind(&MLInpxPlugin::booksDirectoryPathSlot, this,
                    std::placeholders::_1, fd));
    }

  fd->signal_close_request().connect(
      [fd] {
        std::unique_ptr<Gtk::FileChooserDialog> fdl(fd);
        fdl->set_visible(false);
        return true;
      },
      false);

  fd->present();
#endif
}

#ifdef ML_GTK_OLD
void
MLInpxPlugin::inpxFilePathSlot(int respons_id, Gtk::FileChooserDialog *fd)
{
  if(respons_id == Gtk::ResponseType::ACCEPT)
    {
      Glib::RefPtr<Gio::File> fl = fd->get_file();
      if(fl)
        {
          path_to_inpx->set_text(fl->get_path());
        }
    }
  fd->close();
}

void
MLInpxPlugin::booksDirectoryPathSlot(int respons_id,
                                     Gtk::FileChooserDialog *fd)
{
  if(respons_id == Gtk::ResponseType::ACCEPT)
    {
      Glib::RefPtr<Gio::File> fl = fd->get_file();
      if(fl)
        {
          path_to_books->set_text(fl->get_path());
        }
    }
  fd->close();
}
#endif

#ifndef ML_GTK_OLD
void
MLInpxPlugin::inpxFilePathSlot(const Glib::RefPtr<Gio::AsyncResult> &result,
                               const Glib::RefPtr<Gtk::FileDialog> &fd)
{
  Glib::RefPtr<Gio::File> fl;
  try
    {
      fl = fd->open_finish(result);
    }
  catch(Gtk::DialogError &er)
    {
      if(er.code() == Gtk::DialogError::Code::FAILED)
        {
          std::cout << "MLInpxPlugin::inpxFilePathSlot error: " << er.what()
                    << std::endl;
        }
    }
  if(fl)
    {
      path_to_inpx->set_text(fl->get_path());
    }
}

void
MLInpxPlugin::booksDirectoryPathSlot(
    const Glib::RefPtr<Gio::AsyncResult> &result,
    const Glib::RefPtr<Gtk::FileDialog> &fd)
{
  Glib::RefPtr<Gio::File> fl;
  try
    {
      fl = fd->select_folder_finish(result);
    }
  catch(Gtk::DialogError &er)
    {
      if(er.code() == Gtk::DialogError::Code::FAILED)
        {
          std::cout << "MLInpxPlugin::booksDirectoryPathSlot: " << er.what()
                    << std::endl;
        }
    }
  if(fl)
    {
      path_to_books->set_text(fl->get_path());
    }
}
#endif

void
MLInpxPlugin::checkEntries()
{
  std::filesystem::path inpx_path
      = std::filesystem::u8path(path_to_inpx->get_text().c_str());
  std::filesystem::path books_path
      = std::filesystem::u8path(path_to_books->get_text().c_str());
  std::filesystem::path coll_path = af->homePath();
  coll_path /= std::filesystem::u8path(".local/share/MyLibrary/Collections");
  std::string coll_nm = collection_name->get_text();
  coll_path /= std::filesystem::u8path(coll_nm);
  if(!std::filesystem::exists(inpx_path))
    {
      confirmationDialog(inpx_path, books_path,
                         coll_path.filename().u8string(), 1);
      return void();
    }
  if(!std::filesystem::exists(books_path))
    {
      confirmationDialog(inpx_path, books_path,
                         coll_path.filename().u8string(), 2);
      return void();
    }
  if(coll_nm.empty())
    {
      confirmationDialog(inpx_path, books_path,
                         coll_path.filename().u8string(), 3);
      return void();
    }
  if(std::filesystem::exists(coll_path))
    {
      confirmationDialog(inpx_path, books_path,
                         coll_path.filename().u8string(), 4);
      return void();
    }

  confirmationDialog(inpx_path, books_path, coll_path.filename().u8string(),
                     5);
}

void
MLInpxPlugin::confirmationDialog(const std::filesystem::path &inpx_path,
                                 const std::filesystem::path &books_path,
                                 const std::string &coll_name,
                                 const int &variant)
{
  Glib::ustring window_title;
  Glib::ustring lab_txt;

  switch(variant)
    {
    case 1:
      {
        window_title = gettext("Error!");
        lab_txt = gettext("Incorrect path to .inpx file!");
        break;
      }
    case 2:
      {
        window_title = gettext("Error!");
        lab_txt = gettext("Incorrect path to books!");
        break;
      }
    case 3:
      {
        window_title = gettext("Error!");
        lab_txt = gettext("Collection name is empty!");
        break;
      }
    case 4:
      {
        window_title = gettext("Error!");
        lab_txt = gettext("Collection already exists!");
        break;
      }
    case 5:
      {
        window_title = gettext("Confirmation");
        lab_txt = gettext("Are you sure?");
        break;
      }
    default:
      return void();
    }

  Gtk::Window *window = new Gtk::Window;
  window->set_application(main_window->get_application());
  window->set_title(window_title);
  window->set_transient_for(*main_window);
  window->set_modal(true);
  window->set_name("MLwindow");

  Gtk::Grid *grid = Gtk::make_managed<Gtk::Grid>();
  grid->set_halign(Gtk::Align::FILL);
  grid->set_valign(Gtk::Align::FILL);
  grid->set_column_homogeneous(true);
  window->set_child(*grid);

  Gtk::Label *lab = Gtk::make_managed<Gtk::Label>();
  lab->set_margin(5);
  lab->set_halign(Gtk::Align::CENTER);
  lab->set_name("windowLabel");
  lab->set_text(lab_txt);
  grid->attach(*lab, 0, 0, 2, 1);

  if(variant == 5)
    {
      Gtk::Button *yes = Gtk::make_managed<Gtk::Button>();
      yes->set_margin(5);
      yes->set_halign(Gtk::Align::CENTER);
      yes->set_name("applyBut");
      yes->set_label(gettext("Yes"));
      yes->signal_clicked().connect(
          [this, inpx_path, books_path, coll_name, window] {
            std::stringstream strm;
            strm.imbue(std::locale("C"));
            std::string str = thr_num->get_text();
            if(str.empty())
              {
                str = "1";
              }
            strm.str(str);
            int num;
            strm >> num;
            if(num <= 0)
              {
                num = 1;
              }
            CollectionProcessGui *cpg
                = new CollectionProcessGui(main_window, af, num);
            cpg->createWindow(inpx_path, books_path, coll_name);
            window->close();
          });
      grid->attach(*yes, 0, 1, 1, 1);
    }

  Gtk::Button *no = Gtk::make_managed<Gtk::Button>();
  no->set_margin(5);
  no->set_halign(Gtk::Align::CENTER);
  no->set_name("cancelBut");
  no->signal_clicked().connect(std::bind(&Gtk::Window::close, window));
  if(variant == 5)
    {
      no->set_label(gettext("No"));
      grid->attach(*no, 1, 1, 1, 1);
    }
  else
    {
      no->set_label(gettext("Close"));
      grid->attach(*no, 0, 1, 2, 1);
    }

  window->signal_close_request().connect(
      [window] {
        std::unique_ptr<Gtk::Window> win(window);
        win->set_visible(false);
        return true;
      },
      false);

  window->present();
}
