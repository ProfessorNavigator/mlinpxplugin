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
#include <gtkmm-4.0/gtkmm/button.h>
#include <gtkmm-4.0/gtkmm/grid.h>
#include <libintl.h>

#ifdef USE_OPENMP
#include <omp.h>
#endif
#ifndef USE_OPENMP
#include <thread>
#endif

CollectionProcessGui::CollectionProcessGui(Gtk::Window *parent_window,
                                           const std::shared_ptr<AuxFunc> &af,
                                           const int &thr_num)
{
  this->parent_window = parent_window;
  this->af = af;
  this->thr_num = thr_num;
  coll_proc = new CollectionProcess(af, thr_num);
}

CollectionProcessGui::~CollectionProcessGui()
{
  delete coll_proc;
  delete progress_disp;
  delete ops_completed_disp;
}

void
CollectionProcessGui::createWindow(const std::filesystem::path &inpx_path,
                                   const std::filesystem::path &books_path,
                                   const std::string &coll_name)
{
  main_window = new Gtk::Window;
  main_window->set_application(parent_window->get_application());
  main_window->set_transient_for(*parent_window);
  main_window->set_title(gettext("Collection creation"));
  main_window->set_name("MLwindow");
  main_window->set_modal(true);
  main_window->set_deletable(false);
  main_window->set_default_size(1, 1);

  Gtk::Grid *grid = Gtk::make_managed<Gtk::Grid>();
  grid->set_halign(Gtk::Align::FILL);
  grid->set_valign(Gtk::Align::FILL);
  main_window->set_child(*grid);

  Gtk::Label *current_operation = Gtk::make_managed<Gtk::Label>();
  current_operation->set_margin(5);
  current_operation->set_halign(Gtk::Align::CENTER);
  current_operation->set_name("windowLabel");
  current_operation->set_text(gettext("Import progress"));
  grid->attach(*current_operation, 0, 0, 1, 1);

  progress = Gtk::make_managed<Gtk::ProgressBar>();
  progress->set_margin(5);
  progress->set_halign(Gtk::Align::FILL);
  progress->set_name("progressBars");
  progress->set_show_text(true);
  grid->attach(*progress, 0, 1, 1, 1);

  Gtk::Button *cancel = Gtk::make_managed<Gtk::Button>();
  cancel->set_margin(5);
  cancel->set_halign(Gtk::Align::CENTER);
  cancel->set_name("cancelBut");
  cancel->set_label(gettext("Cancel"));
  cancel->signal_clicked().connect([this] {
    canceled = true;
    if(coll_proc)
      {
        coll_proc->stopAll();
      }
  });
  grid->attach(*cancel, 0, 2, 1, 1);

  main_window->signal_close_request().connect(
      [this] {
        std::unique_ptr<Gtk::Window> win(main_window);
        win->set_visible(false);
        delete this;
        return true;
      },
      false);

  main_window->present();

  launchProc(inpx_path, books_path, coll_name);
}

void
CollectionProcessGui::launchProc(const std::filesystem::path &inpx_path,
                                 const std::filesystem::path &books_path,
                                 const std::string &coll_name)
{
  progress_disp = new Glib::Dispatcher;
  progress_disp->connect([this] {
    progress->set_fraction(parsed_bytes.load() / total_size.load());
  });

  ops_completed_disp = new Glib::Dispatcher;
  ops_completed_disp->connect(
      std::bind(&CollectionProcessGui::completeMessage, this));

  coll_proc->signal_progress = [this](const double &pb, const double &ts) {
    parsed_bytes.store(pb);
    total_size.store(ts);
    progress_disp->emit();
  };

#ifndef USE_OPENMP
  std::thread work_thr([this, inpx_path, books_path, coll_name] {
    coll_proc->collectFiles(inpx_path, books_path, coll_name);
    coll_proc->createBase();
    ops_completed_disp->emit();
  });
  work_thr.detach();
#endif
#ifdef USE_OPENMP
#pragma omp masked
  {
    omp_event_handle_t event;
#pragma omp task detach(event)
    {
      coll_proc->collectFiles(inpx_path, books_path, coll_name);
      coll_proc->createBase();
      ops_completed_disp->emit();
      omp_fulfill_event(event);
    }
  }
#endif
}

void
CollectionProcessGui::completeMessage()
{
  main_window->unset_child();
  main_window->set_default_size(1, 1);

  Glib::RefPtr<Glib::MainContext> mc = Glib::MainContext::get_default();
  while(mc->pending())
    {
      mc->iteration(true);
    }

  Gtk::Grid *grid = Gtk::make_managed<Gtk::Grid>();
  grid->set_halign(Gtk::Align::FILL);
  grid->set_valign(Gtk::Align::FILL);
  main_window->set_child(*grid);

  Gtk::Label *lab = Gtk::make_managed<Gtk::Label>();
  lab->set_margin(5);
  lab->set_halign(Gtk::Align::CENTER);
  lab->set_name("windowLabel");
  lab->set_max_width_chars(50);
  lab->set_wrap_mode(Pango::WrapMode::WORD);
  lab->set_justify(Gtk::Justification::CENTER);
  if(canceled)
    {
      lab->set_text(gettext("Operation has been interrupted!"));
    }
  else
    {
      lab->set_text(gettext("All operations completed."));
    }
  grid->attach(*lab, 0, 0, 1, 1);

  Gtk::Button *close = Gtk::make_managed<Gtk::Button>();
  close->set_margin(5);
  close->set_halign(Gtk::Align::CENTER);
  close->set_name("operationBut");
  close->set_label(gettext("Close"));
  close->signal_clicked().connect(std::bind(&Gtk::Window::close, main_window));
  grid->attach(*close, 0, 1, 1, 1);
}
