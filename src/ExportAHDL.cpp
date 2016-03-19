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
#include <qregexp.h>

#include "ExportAHDL.h"
#include "Machine.h"
#include "TransitionInfo.h"
//#include "AppInfo.h"
#include "IOInfo.h"
#include "Options.h"

//using namespace std;


ExportAHDL::ExportAHDL(Options* opt)
  	  : Export(opt)
{
  sync_reset = false;
  use_moore = false;
}

/// Writes all the relevant data into the tdf file.
void ExportAHDL::doExport()
{
  sync_reset=options->getAHDLSyncReset();
  use_moore=options->getAHDLUseMoore();

  writeHeader("%", "%");
  writeName();
  writeIO();
  writeVariables();
  writeMain();
}


QString ExportAHDL::fileFilter()
{
  return "AHDL (*.tdf)";
}

QString ExportAHDL::defaultExtension()
{
	return "tdf";
}

/*
/// Writes the header (some comments) into the file.
void ExportAHDL::writeHeader()
{
  // Write some comment
  QWidget* wm = qApp->mainWidget();
  AppInfo ai(wm);

  *out << "% This AHDL file was generated by	%" << endl;
  *out << "% Qfsm Version " << ai.getVersionMajor() << "." 
       << ai.getVersionMinor() << "			%" << endl;
  *out << "% (C) " << ai.getAuthor() << "			%" << endl << endl;
}
*/



/// Writes the SUBDESIGN line to the output stream
void ExportAHDL::writeName()
{
  using namespace std;

  QString n;
//  int found, count=0;

  n = machine->getName();

  n = n.replace(QRegExp("\\s"), "_");

  /*
  found = n.find(" ", count);
  while (found!=-1)
  {
    n[found]='_';
    count=found+1;
    found = n.find(" ", count);
  }
  */
    
  *out << "SUBDESIGN " << n.latin1() << endl;
}


/// Writes the inputs/outputs to the output stream
void ExportAHDL::writeIO()
{
  using namespace std;

  *out << "(clock, reset		:INPUT;" << endl;
  *out << machine->getMealyInputNames().latin1() << "	:INPUT;" << endl;
  *out << machine->getMealyOutputNames().latin1() << "	:OUTPUT;" << endl;
  
  if (use_moore)
    *out << machine->getMooreOutputNames().latin1() << "	:OUTPUT;" ;
  else
    *out << machine->getStateEncodingOutputNames().latin1() << "	:OUTPUT;" ;

  *out << ")" << endl << endl;
}


/// Writes the variables to the output stream
void ExportAHDL::writeVariables()
{
  using namespace std;

  QString sn, c;
  GState* s;
  bool first=TRUE;

  *out << "VARIABLE" << endl;
  *out << "\tfsm\t:\tMACHINE OF BITS(" << /*machine->getMooreOutputNames()*/
    machine->getStateEncodingOutputNames().latin1() << ")" << endl;
  *out << "\t\t\t\tWITH STATES (";
  
  QMutableListIterator<GState*> i(machine->getSList());

  // Write initial state
//  while (i.current() && !i.current()->isDeleted() && i.current()!=machine->getInitialState())
//    ++i;
  while (i.hasNext() && !i.peekNext()->isDeleted() && i.peekNext()!=machine->getInitialState())
    i.next();


  if (i.hasNext() && !i.peekNext()->isDeleted() && i.peekNext()==machine->getInitialState())
  {
    s = i.peekNext();
    
    sn = s->getStateName();
    sn.replace(QRegExp(" "), "_");
    c = s->getCodeStr(Binary);

    if (!first)
      *out << ",";
    *out << endl << "\t\t\t\t\t";
    *out << sn.latin1() << " = B\"" << c.latin1() << "\""; 
    first=FALSE;
  }

  i.toFront();

  // Write other states
  for(; i.hasNext();)
  {
    s = i.next();
    if (!s->isDeleted() && s != machine->getInitialState())
    {
      sn = s->getStateName();
      sn.replace(QRegExp(" "), "_");
      c = s->getCodeStr(Binary);

      if (!first)
	*out << ",";
      *out << endl << "\t\t\t\t\t";
      *out << sn.latin1() << " = B\"" << c.latin1() << "\""; 
      first=FALSE;
    }
  }
  *out << ");" << endl;

  if (sync_reset)
    *out << "\treset_async : NODE;" << endl; 
  if (use_moore)
    *out << "\t" << machine->getMooreOutputNamesAsync().latin1() << " : NODE;" << endl;

  *out << endl;
}


/// Writes the definition of the subdesign (from BEGIN to END) to the output stream
void ExportAHDL::writeMain()
{
  using namespace std;

  *out << "BEGIN" << endl;

  if (sync_reset)
  {
    *out << "\treset_sync = DFF(reset,clock,VCC,VCC);" << endl;
    *out << "\tfsm.reset = reset_sync;" << endl << endl;
  }
  else
    *out << "\tfsm.reset = reset;" << endl << endl;

  *out << "\tfsm.clk = clock;" << endl;


  if (use_moore)
  {
    QStringList::Iterator it;
    QString name, name_async;
    QStringList output_names_moore = machine->getMooreOutputList();

    for(it = output_names_moore.begin(); it!=output_names_moore.end(); ++it)
    {
      name = *it;
      name_async = name + "_async";

      *out << "\t" << name.latin1() << " = DFF(" << name_async.latin1() << ",clock,VCC,VCC);" << endl;
    }
    *out << endl;
  }

  writeTransitions();

  *out << "END;" << endl;
}


/// Writes the CASE part to the output stream
void ExportAHDL::writeTransitions()
{
  using namespace std;

  GState* s;
  GTransition* t;
  QString tinfoi, tinfoo, sn;
  State* stmp;
  TransitionInfo* tinfo;
  IOInfo* iosingle;
  IOInfo* tioinfo;
//  Convert conv;
  bool first;
  IOInfo* mooreinfo;
  QString smooreinfo;

  *out << "\tCASE fsm IS" << endl;
  
  QMutableListIterator<GState*> is(machine->getSList());

  for(; is.hasNext();)
  {
    s = is.next();
    sn = s->getStateName();
    sn.replace(QRegExp(" "), "_");
    if (s->countTransitions()>0)
      *out << "\t\tWHEN " << sn.latin1() << " =>" << endl;

    QMutableListIterator<GTransition*> it(s->tlist);

    for(; it.hasNext();)
    {
      t = it.next();
      tinfo = t->getInfo();
      tioinfo = tinfo->getInputInfo();

      if (!t->isDeleted() && t->getEnd())
      {
	QList<IOInfo*> iolist;
	iolist = tioinfo->getSingles();
//	iolist.setAutoDelete(TRUE);
	
	QMutableListIterator<IOInfo*> ioit(iolist);


  if(machine->getNumInputs()>0)
        *out << "\t\t\tIF (";

	first = TRUE;
	for(; ioit.hasNext();)
	{

	  iosingle = ioit.next();
	  tinfoi = iosingle->convertToBinStr();  //iosingle->getInputsStrBin();

//	  *out << "\t\t\tIF (" << machine->getMealyInputNames() << ") == B\"";
	  if (!tinfoi.isEmpty())
	  {
	    if (!first)
	    {
	      if (tioinfo->isInverted())
		*out << "AND ";
	      else
		*out << "OR ";
	      *out << endl << "\t\t\t\t";
	    }
	    *out << machine->getMealyInputNames().latin1();
	    if (tioinfo->isInverted())
	      *out << ") != B\"";
	    else
	      *out << ") == B\"";

	    int slen = tinfoi.length();
	    int numin = machine->getNumInputs();
	    for(int k=slen; k<numin; k++)
	      *out << "0";

	    *out << tinfoi.latin1() << "\" ";
	    first=FALSE;
	  }
	}

	tinfoo = tinfo->getOutputsStrBin();

  if(machine->getNumInputs()>0)
    *out << "THEN" << endl;
        // mealy outputs
	if (!tinfoo.isEmpty())
	{
	  *out << "\t\t\t\t(" << machine->getMealyOutputNames().latin1() << ") = B\"";

	  int slen = tinfoo.length();
	  int numout = machine->getNumOutputs();
	  for(int l=slen; l<numout; l++)
	    *out << "0";

	  *out << tinfoo.latin1() << "\";" << endl;
	}
	// next state
	stmp = t->getEnd();
	if (stmp)
        {
	  sn = stmp->getStateName();
          sn.replace(QRegExp(" "), "_");
        }
	if (stmp!=s)
        {
	  *out << "\t\t\t\tfsm = " << sn.latin1() << ";" << endl;

	  if (use_moore)
	  {
            // moore outputs
            mooreinfo = stmp->getMooreOutputs();
            smooreinfo = mooreinfo->convertToBinStr();
	    if (mooreinfo->getLength()>0)
	    {
	      *out << "\t\t\t\t(" <<
		machine->getMooreOutputNamesAsync().latin1() << ") = B\"";
  
	      int slen = smooreinfo.length();
	      int numout = machine->getNumMooreOutputs();
	      for(int l=slen; l<numout; l++)
	        *out << "0";

	      *out << smooreinfo.latin1() << "\";" << endl;
	    }
	  }
	}
  if(machine->getNumInputs()>0)
    *out << "\t\t\tEND IF;" << endl;
      }
    }
  }

  *out << "\tEND CASE;" << endl;
}

