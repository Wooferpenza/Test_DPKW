#ifndef _WX_VALPXPH__
#define _WX_VALPXPH__

// на часть кода копирайт принадлежит Borland Intl.
// based on pictval.cpp from Boland C++ OWL


// from Borland C++ - errors code
enum wxPicResult {
  prComplete,
  prIncomplete,
  prEmpty,
  prError,
  prSyntax,
  prAmbiguous,
  prIncompNoFill
};

class wxPXPictureValidator : public wxValidator
{
public:
   wxPXPictureValidator(const char *pic, wxString *val =0, bool autoFill=false);
   wxPXPictureValidator(const wxPXPictureValidator& val);
    
   ~wxPXPictureValidator();
    
   // Make a clone of this validator (or return NULL) - currently necessary
   // if you're passing a reference to a validator.
   // Another possibility is to always pass a pointer to a new validator
   // (so the calling code can use a copy constructor of the relevant class).
   virtual wxObject *Clone() const { return new wxPXPictureValidator(*this); }
   bool Copy(const wxPXPictureValidator& val);
   
   virtual bool Validate(wxWindow *parent); // Called when the value in the window must be validated. This function can pop up an error message.
   virtual bool TransferToWindow(); // Called to transfer data to the window
   virtual bool TransferFromWindow(); // Called to transfer data to the window
   
   void OnChar(wxKeyEvent& event); // Filter keystrokes
      
private:
   bool IsComplete(wxPicResult rslt);
   bool IsIncomplete(wxPicResult rslt);
   void ToGroupEnd(int termCh, int& i);
   bool SkipToComma(int termCh, int& i);
   int CalcTerm(int termCh, int i);
   wxPicResult Iteration(char *input, int termCh, int& i, int& j);
   wxPicResult Group(char *input, int termCh, int& i, int& j);
   wxPicResult CheckComplete(int termCh, int& i, wxPicResult rslt);
   
   wxPicResult Picture(char *input, bool autoFill=false);

   wxPicResult Scan(char *input, int termCh, int& i, int& j);
   wxPicResult Process(char *input, int termCh, int& i, int& j);
   bool SyntaxCheck();

protected:
   wxString Pic;
   wxString *m_stringValue;
   bool optFill;
   
   const wxString& GetPic() const;
   void SetPic(const wxString& pic);
   
   bool CheckValidator() const
   {
      wxCHECK_MSG( m_validatorWindow, FALSE, _T("No window associated with validator") );
      wxCHECK_MSG( m_validatorWindow->IsKindOf(CLASSINFO(wxTextCtrl)), FALSE, _T("wxPXPictureValidator is only for wxTextCtrl's") );
      wxCHECK_MSG( m_stringValue, FALSE, _T("No variable storage for validator") );
      return TRUE;
   }

DECLARE_EVENT_TABLE()
};

inline const wxString& wxPXPictureValidator::GetPic() const { return Pic;}
inline void wxPXPictureValidator::SetPic(const wxString& pic) { Pic = pic;}

#endif
