#include "wx/wxprec.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "../include/stubs.h"
//#include "wx/xrc/xmlres.h"          // XRC XML resouces

#include "wxfft.h"
#include "osc_dlg.h"
#include "setchan.h"
#include "wx/valgen.h"

extern wxString ChNum[32]; // fill in constructor 0-31 digits
extern wxString KU[];
extern wxString Mode[];

wxSetChanDlg::wxSetChanDlg(wxWindow* parent, LogChan& buffer, PLATA_DESCR_U2 *pd)
:wxDialog(parent, wxID_ANY,"Set logical channel",
          parent->GetScreenPosition()+wxPoint((parent->GetSize()).GetWidth(),(parent->GetSize()).GetHeight()/2))
{    
   wxBoxSizer *mainsizer = new wxBoxSizer(wxHORIZONTAL);

   cbNum = new wxComboBox(this, wxID_ANY, "num",
                         wxPoint(0,0),wxSize(40, wxDefaultCoord),
                         32,ChNum,
                         0,
                         wxGenericValidator(&buffer.Num),"chnum"
                         );
   mainsizer->Add(cbNum,0,wxALIGN_CENTER|wxALL);

   cbMode = new wxComboBox(this, wxID_ANY,"mode",
                         wxPoint(0,0),wxSize(60, wxDefaultCoord),
                         3,Mode,
                         0,
                         wxGenericValidator(&buffer.Mode),"chmode"
                         );
   mainsizer->Add(cbMode,0,wxALIGN_CENTER|wxALL);

   int idx = 0;
   if(!strcmp(pd->t1.BrdName,"L783")) idx=4;
   cbKU = new wxComboBox(this, wxID_ANY,"ku",
                         wxPoint(0,0),wxSize(150, wxDefaultCoord),
                         4,&KU[idx],
                         0,
                         wxGenericValidator(&buffer.KU),"chku"
                         );
   mainsizer->Add(cbKU,0,wxALIGN_CENTER|wxALL);

   wxButton *ok_button = new wxButton(this, wxID_OK, wxT("OK"), wxPoint(0, 0), wxSize(40, 20));
   ok_button->SetDefault();
   mainsizer->Add(ok_button,0,wxALIGN_CENTER|wxALL);

   SetSizer(mainsizer);
   mainsizer->SetSizeHints(this);
}

// Destructor. (Empty, as I don't need anything special done when destructing).
wxSetChanDlg::~wxSetChanDlg()
{
}

bool wxSetChanDlg::TransferDataToWindow()
{
   bool ret = wxDialog::TransferDataToWindow();
   return ret;
}

bool wxSetChanDlg::TransferDataFromWindow()
{ 
   bool ret = wxDialog::TransferDataFromWindow();
   return ret;
}
