#ifndef __CFGDLG__
#define __CFGDLG__

#include "wx/dialog.h"

class wxSetChanDlg : public wxDialog
{
public:
   wxSetChanDlg(wxWindow *parent, LogChan& buffer, PLATA_DESCR_U2 *pd);
   ~wxSetChanDlg();
   
   virtual bool TransferDataFromWindow();
   virtual bool TransferDataToWindow();

protected:
   wxComboBox *cbNum;
   wxComboBox *cbMode;
   wxComboBox *cbKU;

private:
};

#endif
