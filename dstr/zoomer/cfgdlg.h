#ifndef __CFGDLG__
#define __CFGDLG__

#include "wx/dialog.h"

class wxConfigDlg : public wxDialog
{
public:
   wxConfigDlg(wxWindow *parent, ParamBuffer& buffer);
   
   virtual bool TransferDataFromWindow();
   virtual bool TransferDataToWindow();
};

#endif
