#include "wx/wx.h"
#include "wx/utils.h"
#include "valpxp.h"

// на часть кода копирайт принадлежит Borland Intl.
// based on pictval.cpp from Boland C++ OWL

BEGIN_EVENT_TABLE(wxPXPictureValidator, wxValidator)
   EVT_CHAR(wxPXPictureValidator::OnChar)
END_EVENT_TABLE()

wxPXPictureValidator::wxPXPictureValidator(const char *pic, wxString *val, bool autoFill)
{
   Pic = wxString(pic);
   m_stringValue = val;
   if(autoFill) optFill = TRUE;
   if(Picture(0,false)!=prEmpty)
   {
      wxMessageBox("Wrong Pic syntax", _("Validation conflict"), wxOK | wxICON_EXCLAMATION);
      throw;
   }
}

wxPXPictureValidator::wxPXPictureValidator(const wxPXPictureValidator& val)
    : wxValidator()
{
   Copy(val);
}

bool wxPXPictureValidator::Copy(const wxPXPictureValidator& val)
{
   wxValidator::Copy(val);
   Pic = val.Pic;
   m_stringValue=val.m_stringValue;
   optFill = val.optFill;
   return TRUE;
}

wxPXPictureValidator::~wxPXPictureValidator()
{
}

// Called to transfer data to the window
bool wxPXPictureValidator::TransferToWindow(void)
{
   if(!CheckValidator()) return FALSE;
   wxTextCtrl *control = (wxTextCtrl *) m_validatorWindow ;
   control->SetValue(* m_stringValue);
   return TRUE;
}

// Called to transfer data to the window
bool wxPXPictureValidator::TransferFromWindow(void)
{
   if(!CheckValidator()) return FALSE;
   wxTextCtrl *control = (wxTextCtrl *) m_validatorWindow ;
   * m_stringValue = control->GetValue();
   return TRUE;
}

// Called when the value in the window must be validated.
bool wxPXPictureValidator::Validate(wxWindow *parent)
{
   if(!CheckValidator()) return FALSE;

   wxTextCtrl *control = (wxTextCtrl *) m_validatorWindow ;
   if (!control->IsEnabled()) return TRUE; // If window is disabled, simply return
   
   if(Pic.IsNull()) return TRUE; // nothing to validate
   
   wxString val(control->GetValue());

   char *buf;
   buf = new char[val.Len()+1];
   strcpy(buf,val.c_str());
   
   wxPicResult res = Picture(buf,false);
   
   val=buf;
   delete[] buf;
   
   bool ok = (res == prComplete || res == prEmpty);  
   
   wxString errormsg = _("'%s' is invalid input ( %s )"); // NB: this format string should contian exactly one '%s'
/*   
   switch(res)
   {
      case prComplete: ok = TRUE; break;
      case prIncomplete:  errormsg = _("'%s' is prIncomplete"); break;
      case prEmpty: errormsg = _("'%s' is prEmpty"); break;
      case prError: errormsg = _("'%s' is prError"); break;
      case prSyntax: errormsg = _("'%s' is prSyntax"); break;
      case prAmbiguous: errormsg = _("'%s' is prAmbiguous"); break;
      case prIncompNoFill: errormsg = _("'%s' is prIncompNoFill"); break;
   }
*/
   
   if(!ok)
   {
      wxASSERT_MSG( !errormsg.empty(), _T("you forgot to set errormsg") );
      m_validatorWindow->SetFocus();
      wxString buf;
      buf.Printf(errormsg, val.c_str(), Pic.c_str());
      wxMessageBox(buf, _("Validation conflict"), wxOK | wxICON_EXCLAMATION, parent);
   }

   control->SetValue(val);
   control->SetInsertionPointEnd();

   return ok;
}

void wxPXPictureValidator::OnChar(wxKeyEvent& event)
{
   if( m_validatorWindow )
   {
      int keyCode = (int)event.GetKeyCode();
      
      if( (keyCode!=WXK_BACK) && (!Pic.IsNull()) )
      {
         wxTextCtrl *control = (wxTextCtrl *) m_validatorWindow ;
         wxString val(control->GetValue());
         if(keyCode<256) if(isprint(keyCode)) val = val + (wxChar)keyCode;

         char *buf;
         buf = new char[val.Len()+1];
         strcpy(buf,val.c_str());
    
         wxPicResult res = Picture(buf,optFill);
         
         val = buf;
         delete[] buf;
         
         bool ok = (res!=prError);

         wxString errormsg = _("'%s' is invalid input ( %s )"); // NB: this format string should contian exactly two '%s'
         
/*         
         switch(res)
         {
            case prComplete: ok = TRUE; break;
            case prIncomplete:  errormsg = _("'%s' is prIncomplete"); ok = TRUE; break;
            case prEmpty: errormsg = _("'%s' is prEmpty"); ok = FALSE; break;
            case prError: errormsg = _("'%s' is prError"); ok = FALSE; break;
            case prSyntax: errormsg = _("'%s' is prSyntax"); ok = FALSE; break;
            case prAmbiguous: errormsg = _("'%s' is prAmbiguous"); ok = FALSE; break;
            case prIncompNoFill: errormsg = _("'%s' is prIncompNoFill"); ok = FALSE; break;
         }
*/  
         if(!ok)
         {
         /*
            wxASSERT_MSG( !errormsg.empty(), _T("you forgot to set errormsg") );
            m_validatorWindow->SetFocus();
            wxString buf;
            buf.Printf(errormsg, val.c_str(),Pic.c_str());
            wxMessageBox(buf, _("Validation conflict"), wxOK | wxICON_EXCLAMATION);
         */  
            if(!wxValidator::IsSilent()) wxBell();
            return;
         }
         
         control->SetValue(val);
         control->SetInsertionPointEnd();
         return;
      }
   }
   event.Skip();
}

// crossported from borland C++ pictval.cpp

inline bool wxPXPictureValidator::IsComplete(wxPicResult rslt) { return rslt == prComplete || rslt == prAmbiguous; }
inline bool wxPXPictureValidator::IsIncomplete(wxPicResult rslt) { return rslt == prIncomplete || rslt == prIncompNoFill; }

void wxPXPictureValidator::ToGroupEnd(int termCh, int& i)
{
   int brkLevel = 0;
   int brcLevel = 0;

   do
   {
      if(i == termCh) return;
      switch(Pic[i])
      {
         case '[': brkLevel++; break;
         case ']': brkLevel--; break;
         case '{': brcLevel++; break;
         case '}': brcLevel--; break;
         case ';': i++; break;
         case '*':
                  i++;
                  while (isdigit(Pic[i])) i++;    // correct
                  ToGroupEnd(termCh, i);
                  continue;
      }
      i += sizeof(wxChar);
   } while (brkLevel || brcLevel);
}

bool wxPXPictureValidator::SkipToComma(int termCh, int& i)
{
   while(1)
   {
      ToGroupEnd(termCh, i);
      if(i==termCh) return false;
      if(Pic[i]==',') {i++; return i<termCh; }
   }
}

int wxPXPictureValidator::CalcTerm(int termCh, int i)
{
   ToGroupEnd(termCh, i);
   return i;
}

wxPicResult wxPXPictureValidator::Iteration(char *input, int termCh, int& i, int& j)
{
   wxPicResult rslt;
   int newTermCh;

   i++;  // Skip '*'

   // Retrieve number

   int itr = 0;
   for (; isdigit(Pic[i]); i++) itr=itr*10+Pic[i]-'0';

   if(i >= termCh) return prSyntax;

   newTermCh = CalcTerm(termCh, i);

   //
   // if itr is 0 allow any number, otherwise enforce the number
   //
   int k = i;
   if(itr)
   {
      for (int m = 0; m < itr; m++)
      {
         i=k;
         rslt=Process(input, newTermCh, i, j);
         if(!IsComplete(rslt))
         {
            if(rslt==prEmpty) rslt = prIncomplete; // Empty means incomplete since all are required
            return rslt;
         }
      }
   }
   else
   {
      do
      {
         i=k;
         rslt=Process(input, newTermCh, i, j);
      } while (IsComplete(rslt));
      
      if(rslt==prEmpty || rslt==prError) { i++; rslt = prAmbiguous; }
   }
   i = newTermCh;
   return rslt;
}

wxPicResult wxPXPictureValidator::Group(char *input, int termCh, int& i, int& j)
{
   int groupTermCh = CalcTerm(termCh, i);
   i++;
   wxPicResult rslt=Process(input, groupTermCh - 1, i, j);
   if(!IsIncomplete(rslt)) i=groupTermCh;
   return rslt;
}

wxPicResult wxPXPictureValidator::CheckComplete(int termCh, int& i, wxPicResult rslt)
{
   int j = i;
   if(IsIncomplete(rslt))
   {
      // Skip optional pieces
      while(1)
      {
         if (Pic[j] == '[') ToGroupEnd(termCh, j);
         else
            if (Pic[j] == '*')
            {
               if(!isdigit(Pic[j+1])) { j++; ToGroupEnd(termCh, j); }
               else break;
            }
            else break;

         if(j==termCh) return prAmbiguous;  // end of the string, don't know if complete
      }
   }
   return rslt;
}

wxPicResult wxPXPictureValidator::Scan(char *input, int termCh, int& i, int& j)
{
  char ch;
  wxPicResult rslt = prEmpty;

  unsigned len = strlen(input);
  while (i != termCh && Pic[i] != ',') {
    if(j>=len) return CheckComplete(termCh, i, rslt);

    ch = input[j];
    switch (Pic[i]) {
      case '#':
        if (!isdigit(ch)) return prError;
        else { input[j++] = ch; i++; }
        break;
      case '?':
        if (!isalpha(ch)) return prError;
        else { input[j++] = ch; i++; }
        break;
      case '&':
        if (!isalpha(ch)) return prError;
        else { input[j++] = (char)wxToupper(ch); i++; }
        break;
      case '!': {
        input[j++] = (char)wxToupper(ch);
        i++;
        break;
      }
      case '@': {
        input[j++] = ch;
        i++;
        break;
      }
      case '*':
        rslt = Iteration(input, termCh, i, j);
        if (!IsComplete(rslt)) return rslt;
        if (rslt == prError) rslt = prAmbiguous;
        break;
      case '{':
        rslt = Group(input, termCh, i, j);
        if (!IsComplete(rslt)) return rslt;
        break;
      case '[':
        rslt = Group(input, termCh, i, j);
        if (IsIncomplete(rslt)) return rslt;
        if (rslt == prError) rslt = prAmbiguous;
        break;
      default: {
        if (Pic[i] == ';') i++;
        if (wxToupper(Pic[i]) != wxToupper(ch))
          if (ch == ' ') ch = Pic[i];
          else return prError;
        input[j++] = Pic[i];
        i++;
      }
    }
    if (rslt == prAmbiguous) rslt = prIncompNoFill;
    else rslt = prIncomplete;
  }

  return (rslt == prIncompNoFill) ? prAmbiguous : prComplete;
}

wxPicResult wxPXPictureValidator::Process(char *input, int termCh, int& i, int& j)
{
   wxPicResult rslt;
   int incompJ, incompI;
   incompJ = incompI = 0;

   bool incomp = false;
   int oldI = i;
   int oldJ = j;
   do
   {
      rslt = Scan(input, termCh, i, j);

      //
      // Only accept completes if they make it farther in the input
      // stream from the last incomplete
      //
      if ((rslt==prComplete || rslt==prAmbiguous) && incomp && j < incompJ)
      {
         rslt = prIncomplete;
         j = incompJ;
      }

      if (rslt == prError || rslt == prIncomplete)
      {
         if (!incomp && rslt == prIncomplete)
         {
            incomp = true;
            incompI = i;
            incompJ = j;
         }
         i = oldI;
         j = oldJ;
         if (!SkipToComma(termCh, i))
         {
            if (incomp)
            {
               i = incompI;
               j = incompJ;
               return prIncomplete;
            }
            return rslt;
         }
      oldI = i;
      }
   } while (rslt == prError || rslt == prIncomplete);

   return (rslt == prComplete && incomp) ? prAmbiguous : rslt;
}

bool wxPXPictureValidator::SyntaxCheck()
{
   if (Pic.IsNull()) return false;
   if (Pic[Pic.Length()-1] == ';') return false;
   if (Pic[Pic.Length()-1] == '*' && Pic[Pic.Length()-2] != ';') return false;

   int brkLevel = 0;
   int brcLevel = 0;
   for (int i = 0; i < Pic.Length(); )
   {
      switch (Pic[i])
      {
         case '[': brkLevel++; break;
         case ']': brkLevel--; break;
         case '{': brcLevel++; break;
         case '}': brcLevel--; break;
         case ';': i++;
      }
      i += sizeof(wxChar);
   }
   return !(brkLevel || brcLevel);
}

wxPicResult wxPXPictureValidator::Picture(char *input, bool autoFill)
{
   if(!SyntaxCheck()) return prSyntax;
   if(!input || !*input) return prEmpty;

   int j = 0;  // index for input[]
   int i = 0;  // index for Pic[]

   wxPicResult rslt = Process(input, Pic.Length(), i, j);
   if(rslt != prError && rslt != prSyntax && j < strlen(input)) rslt = prError;

   // If the result is incomplete & autofill is requested, then copy literal
   // characters from the picture over to the input.
   //
   if(rslt == prIncomplete && autoFill)
   {
      bool  reprocess = false;
      while (i < Pic.Length() && !strchr("#?&!@*{}[],", Pic[i]))
      {
         if (Pic[i] == ';') i++;
         input[j++] = Pic[i++];
         reprocess = true;
      }

      if (reprocess)
      {
         input[j] = 0;   // terminate the copy, since we are probably appending
         j = i = 0;
         rslt = Process(input, Pic.Length(), i, j);
      }
   }

   return (rslt == prAmbiguous) ? prComplete : (rslt == prIncompNoFill) ? prIncomplete : rslt;
}
