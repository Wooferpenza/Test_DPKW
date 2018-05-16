#include "wx/wxprec.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

//#include "wx/xrc/xmlres.h"          // XRC XML resouces

#include "toolbox.h"
#include "min_info.h"
#include "hex_info.h"
#include "viewer.h"
#include "cfgdlg.h"
#include "wx/valgen.h"
#include "valpxp.h"
#include <stdlib.h>
#include <string.h>

#include "minimal_bmp_0.xpm"
#include "minimal_bmp_1.xpm"
#include "minimal_bmp_2.xpm"
#include "minimal_bmp_3.xpm"
#include "minimal_bmp_4.xpm"
#include "minimal_bmp_5.xpm"
#include "minimal_bmp_6.xpm"
#include "minimal_bmp_7.xpm"
#include "minimal_bmp_8.xpm"


wxConfigDlg::wxConfigDlg(wxWindow* parent, ParamBuffer& buffer)
:wxDialog(parent, wxID_ANY,"Configure zoomer",
          parent->GetScreenPosition()+wxPoint((parent->GetSize()).GetWidth(),(parent->GetSize()).GetHeight()/2))
{  
   wxString FontSize[]={"8","10","12"};
   wxString xps[] = {"1K","2K","4K","8K","16K","32K","65K","128K"};

   wxFlexGridSizer *mainsizer = new wxFlexGridSizer(0,2,0,0);

   wxFlexGridSizer *leftsizer = new wxFlexGridSizer(4,0,0,0);

   leftsizer->Add(new wxRadioBox(this, -1, "Font size", wxDefaultPosition, wxDefaultSize, 3, FontSize, 1, wxRA_SPECIFY_COLS,
                                 wxGenericValidator(&buffer.FontSize)), 0, wxBOTTOM, 5);

   wxComboBox *cbNCh = new wxComboBox(this, wxID_ANY, "",
                         wxDefaultPosition, wxDefaultSize,
                         0,NULL, wxALIGN_CENTER_HORIZONTAL|wxALL, wxGenericValidator(&buffer.NCh));
   for(int i=1;i<32;i++)
   {
      char str[10]; sprintf(str,"%d",i);
      cbNCh->Append(wxT(str));
   }

   leftsizer->Add(new wxStaticText(this, -1, "Channels"));
   leftsizer->Add(cbNCh);

   wxButton *ok_button = new wxButton(this, wxID_OK, wxT("OK"), wxDefaultPosition, wxDefaultSize);
   ok_button->SetDefault();
   leftsizer->Add(ok_button,0,wxTOP,15);

   wxStaticBoxSizer *rightsizer = new wxStaticBoxSizer(new wxStaticBox(this,wxID_ANY,"X and Y scale"), wxVERTICAL);

   wxFlexGridSizer *fgs1 = new wxFlexGridSizer(0,2,0,0);

   wxFlexGridSizer *fgs2 = new wxFlexGridSizer(6,0,5,5);
   fgs2->Add(new wxStaticText(this, wxID_ANY, "Y max data value"));
   fgs2->Add(new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
                            wxPXPictureValidator("[-]*#",&buffer.MaxData,false)));

   fgs2->Add(new wxStaticText(this, wxID_ANY, "Y max scale"));
   fgs2->Add(new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
                            wxPXPictureValidator("[-]*#[;.#]",&buffer.YMax)));

   fgs2->Add(new wxStaticText(this, wxID_ANY, "Y text"));
   fgs2->Add(new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,  wxDefaultSize, 0,
                            wxGenericValidator(&buffer.YText)));

   wxFlexGridSizer *fgs3 = new wxFlexGridSizer(6,0,5,5);
   fgs3->Add(new wxStaticText(this, wxID_ANY, "X text"));
   fgs3->Add(new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,  wxDefaultSize, 0,
                            wxGenericValidator(&buffer.XText)));

   fgs3->Add(new wxStaticText(this, wxID_ANY, "X multi"));
   fgs3->Add(new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,  wxDefaultSize, 0,
                            wxGenericValidator(&buffer.Multi)));

   fgs3->Add(new wxStaticText(this, wxID_ANY, "X page size (points)"));

   fgs3->Add(new wxComboBox(this, wxID_ANY, "",
                          wxDefaultPosition, wxDefaultSize,
                          8, xps, 0, wxGenericValidator(&buffer.XPageSize1024)));

   fgs1->Add(fgs2,0,wxGROW|wxALL,5);
   fgs1->Add(fgs3,0,wxGROW|wxALL,5);

   rightsizer->Add(fgs1);

   mainsizer->Add(leftsizer,0,wxGROW|wxALL,5);
   mainsizer->Add(rightsizer,0,wxGROW|wxTOP|wxBOTTOM|wxRIGHT,5);

   SetSizer(mainsizer);
   mainsizer->SetSizeHints(this);
}

bool wxConfigDlg::TransferDataToWindow()
{
   bool ret = wxDialog::TransferDataToWindow();
   return ret;
}

bool wxConfigDlg::TransferDataFromWindow()
{ 
   bool ret = wxDialog::TransferDataFromWindow();
   return ret;
}


/////////////////////////// toolbar creation ////////////////////////////////////////

void MyDecorFrame::PopulateToolbar(wxToolBar* tb)
{
   wxBitmap tb_bmp = wxBitmap(minimal_bmp_5);
   int w = tb_bmp.GetWidth(),
       h = tb_bmp.GetHeight();
   tb->SetToolBitmapSize(wxSize(w, h));

   tb->AddTool(ID_OPEN_FILE, _T("Open File"), wxBitmap(minimal_bmp_5), _T("Open File"));
   tb->AddTool(ID_SAVE_FILE, _T("Save fragment"), wxBitmap(minimal_bmp_6), _T("Save fragment"));
   tb->AddSeparator();
   tb->AddTool(ID_INFO, _T("Show File/Data info"), wxBitmap(minimal_bmp_4), _T("Show File/Data info"));
   tb->AddTool(ID_HEX_VIEW, _T("Table view"), wxBitmap(minimal_bmp_7), _T("Table view"));
   tb->AddSeparator();
   tb->AddTool(ID_CONFIG, _T("Configure zoomer"), wxBitmap(minimal_bmp_1), _T("Configure zoomer"));
   tb->AddSeparator();
   tb->AddTool(ID_CH_CFG, _T("Configure channels"), wxBitmap(minimal_bmp_2), _T("Configure channels"));
   tb->AddTool(ID_RESET_OFFS, _T("Reset channel offset"), wxBitmap(minimal_bmp_0), _T("Reset channel offset"));

   cb = new wxComboBox(tb,ID_ACH,"",wxDefaultPosition, wxDefaultSize,
                          0, NULL, wxCB_DROPDOWN|wxCB_READONLY);
   tb->AddControl(cb);

   tb->AddSeparator();
   tb->AddTool(ID_SWITCH_BW, _T("Switch BW/Color view mode"), wxBitmap(minimal_bmp_8), _T("Switch BW/Color view mode"));
   tb->AddSeparator();
   tb->AddTool(ID_HELP, _T("Open Help"), wxBitmap(minimal_bmp_3), _T("Open Help"));

   tb->Realize();
}


void MyDecorFrame::PopulateMenubar(wxMenuBar* mb)
{
   // Make a menubar
   wxMenu *fileMenu = new wxMenu;
   fileMenu->Append(ID_OPEN_FILE, "&Open file\tCtrl-O", "Open data file" );
   fileMenu->Append(ID_CLOSE_FILE, "&Close file", "Close data file" );
   fileMenu->Append(ID_SAVE_FILE, "&Save fragment\tCtrl-S", "Save data fragment in file" );
   fileMenu->AppendSeparator();
   fileMenu->Append(ID_PRINT, "&Print", "Print" );
   fileMenu->Append(ID_PAGE_SETUP, "Page setup", "Page setup" );
   fileMenu->Append(ID_PRINT_SETUP, "Print setup", "Print setup" );
   fileMenu->Append(ID_PRINT_PREVIEW, "P&rint preview\tCtrl-P", "Print preview" );
   fileMenu->AppendSeparator();
   fileMenu->Append(ID_QUIT, "&Exit\tCtrl-Q", "Exit program" );

   wxMenu *viewMenu = new wxMenu;
   viewMenu->Append(ID_RESET_OFFS, "&Reset offset\tShift-R", "Reset channel offset" );
   viewMenu->AppendSeparator();
   viewMenu->Append(ID_INFO, "File/Data &info\tF2", "Show file/data info" );
   viewMenu->Append(ID_HEX_VIEW, "&Table view\tF3", "Show data in table mode" );
   viewMenu->AppendSeparator();
   viewMenu->Append(ID_SWITCH_BW, "Color/&BW switch\tF8", "Switch Color/BW mode" );

   wxMenu *configMenu = new wxMenu;
   configMenu->Append(ID_CONFIG, "Configure &zoomer\tShift-Z", "Configure zoomer" );
   configMenu->Append(ID_CH_CFG, "Configure c&hannels\tShift-C", "Configure channels" );
   configMenu->Append(ID_SAVE_CFG, "Save confi&g\tShift-F9", "Save config" );

   wxMenu *pluginMenu = new wxMenu;
   pluginMenu->Append(ID_DAQ_CONN, "Connect to D&AQ server\tShift-A", "Connect to DAQ server" );
   pluginMenu->Append(ID_DAQ_DCONN, "Disconnect from DA_Q server\tShift-Q", "Disconnect from DAQ server" );

   wxMenu *helpMenu = new wxMenu;
   helpMenu->Append(ID_HELP, "&Help c&ontents\tF1", "Help contents");
   helpMenu->Append(ID_ABOUT, "&About Viewer", "About Viewer");
  

   mb->Append(fileMenu, "&File");
   mb->Append(viewMenu, "&View");
   mb->Append(configMenu, "&Configure");
   mb->Append(pluginMenu, "&DAQ");
   mb->Append(helpMenu, "&Help");
}
