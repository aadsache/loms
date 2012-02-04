//---------------------------------------------------------------------------------------
// This file is part of the Lomse library.
// Copyright (c) 2010-2012 Cecilio Salmeron. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice, this 
//      list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright notice, this
//      list of conditions and the following disclaimer in the documentation and/or
//      other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY 
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
// SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
//
// For any comment, suggestion or feature request, please contact the manager of
// the project at cecilios@users.sourceforge.net
//---------------------------------------------------------------------------------------

#ifndef __LOMSE_SHAPE_TEXT_H__        //to avoid nested includes
#define __LOMSE_SHAPE_TEXT_H__

#include "lomse_shape_base.h"
#include "lomse_basic.h"
#include "lomse_injectors.h"
#include <string>
using namespace std;

namespace lomse
{

//forward declarations
class GmoBox;
class ImoStyle;
class FontStorage;

//---------------------------------------------------------------------------------------
class GmoShapeText : public GmoSimpleShape
{
protected:
    std::string m_text;
    ImoStyle* m_pStyle;
    FontStorage* m_pFontStorage;
    LibraryScope& m_libraryScope;

public:
    GmoShapeText(ImoObj* pCreatorImo, int idx, const std::string& text,
                 ImoStyle* pStyle, LUnits x, LUnits y, LibraryScope& libraryScope);
    virtual ~GmoShapeText() {}

    void on_draw(Drawer* pDrawer, RenderOptions& opt);

    //dynamic modification
    void set_text(const std::string& text);


protected:
    void select_font();
    Color get_normal_color();

};

//---------------------------------------------------------------------------------------
class GmoShapeWord : public GmoSimpleShape
{
protected:
    const std::string m_text;
    ImoStyle* m_pStyle;
    FontStorage* m_pFontStorage;
    LibraryScope& m_libraryScope;
    LUnits m_halfLeading;
    LUnits m_baseline;

public:
    GmoShapeWord(ImoObj* pCreatorImo, int idx, const std::string& text,
                 ImoStyle* pStyle, LUnits x, LUnits y,
                 LUnits halfLeading, LibraryScope& libraryScope);
    virtual ~GmoShapeWord() {}

    void on_draw(Drawer* pDrawer, RenderOptions& opt);

    //for unit tests
    inline LUnits get_top_line() { return m_origin.y + m_halfLeading; }
    inline LUnits get_baseline() { return m_baseline; }
    inline LUnits get_bottom_line() { return m_origin.y + m_size.height - m_halfLeading; }

protected:
    void select_font();
    Color get_normal_color();

};

////------------------------------------------------------------------------------------
//// GmoShapeTitle: a primitive textbox. To be replaced, in future, by a GmoShapeTextbox
//
//class GmoShapeTitle : public GmoShapeRectangle
//{
//public:
//    GmoShapeTitle(lmScoreObj* pOwner, const std::string& sText, wxFont* pFont,
//                     lmPaper* pPaper, lmEBlockAlign nBlockAlign,
//                     lmEHAlign nHAlign, lmEVAlign nVAlign,
//                     LUnits xLeft, LUnits yTop,
//                     LUnits xRight, LUnits yBottom,
//                     wxColour nColor = *wxBLACK, std::string sName=_T("ShapeTitle"),
//					 bool fDraggable = true);
//
//    ~GmoShapeTitle() {}
//
//    //implementation of virtual methods from base class
//    void Render(lmPaper* pPaper, wxColour color = *wxBLACK);
//    std::string Dump(int nIndent);
//    void Shift(LUnits xIncr, LUnits yIncr);
//
//    //specific methods
//    void SetFont(wxFont *pFont);
//    std::string* GetText() { return &m_text; }
//
//    //call backs
//    wxBitmap* OnBeginDrag(double rScale, wxDC* pDC);
//	UPoint OnDrag(lmPaper* pPaper, const UPoint& uPos);
//	void OnEndDrag(lmPaper* pPaper, lmInteractor* pCanvas, const UPoint& uPos);
//
//
//
//private:
//    void Create(const std::string& sText, wxFont* pFont, lmPaper* pPaper,
//                lmEBlockAlign nBlockAlign, lmEHAlign nHAlign, lmEVAlign nVAlign,
//                LUnits xLeft, LUnits yTop, LUnits xRight, LUnits yBottom);
//
//    void ComputeTextPosition(lmPaper* pPaper);
//    void ComputeBlockBounds(LUnits xLeft, LUnits yTop, LUnits xRight, LUnits yBottom);
//
//    lmEBlockAlign   m_nBlockAlign;
//    lmEHAlign       m_nHAlign;
//    lmEVAlign       m_nVAlign;
//    std::string        m_text;
//	std::string		m_sClippedText;
//    wxFont*         m_pFont;
//    UPoint        m_uTextPos;     // text position (relative to top-left of rectangle)
//    LUnits        m_uTextWidth;
//    LUnits        m_uTextHeight;
//    LUnits        m_uClippedTextWidth;
//    LUnits        m_uClippedTextHeight;
//
//};
//
//
////------------------------------------------------------------------------------------
//
//class TextLine;
//
//class GmoShapeTextbox : public GmoShapeRectangle
//{
//public:
//    GmoShapeTextbox(lmScoreObj* pOwner, int nShapeIdx,
//            //text related
//            lmPaper* pPaper,
//            const std::string& sText, wxFont* pFont, wxColour nTxtColor,
//            //block related
//            lmEBlockAlign nBlockAlign, lmEHAlign nHAlign, lmEVAlign nVAlign,
//            LUnits xLeft, LUnits yTop, LUnits xRight, LUnits yBottom,
//            wxColour nBgColor = *wxWHITE,
//            //border
//            LUnits uBorderWidth = 1.0f,
//            wxColour nBorderColor = *wxBLACK,
//            lmELineStyle nBorderStyle = lm_eLine_None,             //no border
//            //other
//            std::string sName=_T("ShapeTextbox"),
//            bool fDraggable = true);
//
//    ~GmoShapeTextbox();
//
//    //implementation of virtual methods from base class
//
//    //renderization
//    void Render(lmPaper* pPaper, wxColour color = *wxBLACK);
//    void RenderWithHandlers(lmPaper* pPaper);
//
//    //other
//    std::string Dump(int nIndent);
//
//
//    //specific methods
//    void SetFont(wxFont *pFont);
//    std::string* GetText() { return &m_text; }
//
//    //shape dragging: overrides of GmoShapeRectangle
//    wxBitmap* OnBeginDrag(double rScale, wxDC* pDC);
//
//
//private:
//    void ComputeTextPosition(lmPaper* pPaper);
//    void DrawTextbox(lmPaper* pPaper, wxColour colorC, bool fSketch);
//    LUnits ApplyHAlign(LUnits uAvailableWidth, LUnits uLineWidth, lmEHAlign nHAlign);
//    void DeleteTextLines();
//
//
//    lmEBlockAlign   m_nBlockAlign;
//    lmEHAlign       m_nHAlign;
//    lmEVAlign       m_nVAlign;
//    std::string        m_text;
//    wxFont*         m_pFont;
//    wxColour        m_nTxtColor;
//    UPoint        m_uTextPos;     // text position (relative to top-left of rectangle)
//    LUnits        m_uTextWidth;
//    LUnits        m_uTextHeight;
//
//
//    std::list<TextLine*>   m_TextLines;
//};


}   //namespace lomse

#endif    // __LOMSE_SHAPE_TEXT_H__

