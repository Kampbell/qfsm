/*
Copyright (C) 2000,2001 Stefan Duffner 

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/


#include <iostream>
#include <qapplication.h>
#include "Export.h"
#include "Machine.h"
#include "AppInfo.h"


//using namespace std;

/// Standard constructor.
Export::Export(Options* opt) : QObject()
{
  options = opt;
  out = &std::cout;
  machine = NULL;
  scrollview = NULL;
}

/// Destructor
Export::~Export() 
{
}


/**
 * Initializes the export object.
 * @a o is a pointer to the output stream opened with the standard fstream methods.
 */
//void Export::init(std::ofstream* o, Machine* m, QString fn/*=QString::null*/, ScrollView* sv/*=NULL*/)
void Export::init(std::ostream* o, Machine* m, QString fn/*=QString::null*/, ScrollView* sv/*=NULL*/)
{
  out = o;
  machine = m;
  fileName = fn;
  scrollview = sv;
}

/// Writes the header (some comments) into the file.
void Export::writeHeader(QString commentstart, QString commentend)
{
  using namespace std;

  // Write some comment
  QWidget* wm = qApp->mainWidget();
  AppInfo ai(wm);

  *out << commentstart.latin1() << " This file was generated by				" <<
    commentend.latin1() << endl;
  *out << commentstart.latin1() << " Qfsm Version " << ai.getVersionMajor() << "." 
       << ai.getVersionMinor() << "					" << commentend.latin1() << endl;
  *out << commentstart.latin1() << " (C) " << ai.getAuthor().latin1() << "			" 
       << commentend.latin1() << endl << endl;

}