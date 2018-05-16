#ifndef __HEX_INFO__
#define __HEX_INFO__

#include "wx/minifram.h"

#include "wx/grid.h"
#include "wx/generic/gridctrl.h"


// Define a new mini frame
class MyDecorFrame;
class MyPanel;

class BigGridTable : public wxGridTableBase
{
public:
   BigGridTable(wxFrame *parent, long row, long col)
   { 
      prnt = (MyDecorFrame *)parent;
      m_sizeRow = row;
      m_sizeCol = col;
      view_filter = wxString("%08X");//wxT("%08X");
   }

   int GetNumberRows() { return m_sizeRow; }
   int GetNumberCols() { return m_sizeCol; }

   bool AppendCols(size_t newcol)
   {
      m_sizeCol = newcol;
      if ( GetView() )
      {
         wxGridTableMessage msg( this,
                                wxGRIDTABLE_NOTIFY_COLS_INSERTED,
                                0,
                                newcol );
         GetView()->ProcessTableMessage( msg );
      }

      return true;
   }

   bool AppendRows(size_t newrow)
   { 
      m_sizeRow = newrow; 
      if ( GetView() )
      {
         wxGridTableMessage msg( this,
                                wxGRIDTABLE_NOTIFY_ROWS_INSERTED,
                                0,
                                newrow );
         GetView()->ProcessTableMessage( msg );
      }
      return true;
   }

   bool DeleteCols(size_t pos, size_t numCols)
   {
      if(GetView())
      {
         wxGridTableMessage msg( this,
                                wxGRIDTABLE_NOTIFY_COLS_DELETED,
                                pos,
                                numCols);
         GetView()->ProcessTableMessage( msg );
      }
      m_sizeCol = 0;
      return TRUE;
   }

   bool DeleteRows(size_t pos, size_t numRows)
   {
      if(GetView())
      {
         wxGridTableMessage msg( this,
                                wxGRIDTABLE_NOTIFY_ROWS_DELETED,
                                pos,
                                numRows);
         GetView()->ProcessTableMessage( msg );
      }
      m_sizeRow = 0;
      return TRUE;
   }


   wxString GetValue( int row, int col );
   wxString GetColLabelValue( int col) { return wxString::Format("Channel %ld",col); }
   wxString GetRowLabelValue( int row);// { return wxString::Format("%ld",row); }

   void SetValue( int , int , const wxString&  ) { /* ignore */ }
   bool IsEmptyCell( int , int  ) { return FALSE; }
   void SetFilter(int f);

public:
   long m_sizeRow;
   long m_sizeCol;
   wxString view_filter;
   MyDecorFrame *prnt;

private:
};

class MyHexFrame: public wxMiniFrame
{
public:
   MyHexFrame(wxFrame *parent, wxWindowID id = -1, const wxString& title = "wxToolBar Sample",
        const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize );
   ~MyHexFrame()
   {
//      delete m_grid;
      //delete m_rb;
//      delete m_table; // ÎÑÒÀËÜÍÎÅ ÏÀÏÈÊÈ ÓÁÈÂÀÞÒ....
//      delete m_panel;
   };

   void OnCloseWindow(wxCloseEvent& event)
   {
      Show(0);
   };

   void OnSize(wxSizeEvent& event);
   void OnEraseBackground(wxEraseEvent& event) {};
   void OnChangeView(wxCommandEvent &event);
   void OnEsc(wxKeyEvent& event);
    
//   void SetListItems();
   void UpdateTableSize(int col, int row);
    
   MyPanel *m_panel;
   wxGrid *m_grid;
   wxRadioBox *m_rb;
   BigGridTable* m_table;

private:

DECLARE_EVENT_TABLE()
};

class MyPanel: public wxPanel
{
public:
   MyPanel(wxWindow* parent) :wxPanel(parent) {};
   void OnRadio( wxCommandEvent &event ) { ((MyHexFrame *)GetParent())->OnChangeView(event); };
DECLARE_EVENT_TABLE()
};


#endif
