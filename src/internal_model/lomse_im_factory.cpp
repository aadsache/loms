//---------------------------------------------------------------------------------------
//  This file is part of the Lomse library.
//  Copyright (c) 2010-2011 Lomse project
//
//  Lomse is free software; you can redistribute it and/or modify it under the
//  terms of the GNU General Public License as published by the Free Software Foundation,
//  either version 3 of the License, or (at your option) any later version.
//
//  Lomse is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
//  PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License along
//  with Lomse; if not, see <http://www.gnu.org/licenses/>.
//
//  For any comment, suggestion or feature request, please contact the manager of
//  the project at cecilios@users.sourceforge.net
//
//---------------------------------------------------------------------------------------

#include "lomse_im_factory.h"

#include "lomse_internal_model.h"
#include "lomse_im_note.h"
//#include "lomse_parser.h"
//#include "lomse_analyser.h"
#include "lomse_document.h"


namespace lomse
{

//---------------------------------------------------------------------------------------
ImoObj* ImFactory::inject(Document* pDoc, const std::string& ldpSource)
{
    return pDoc->create_object(ldpSource);
}

//---------------------------------------------------------------------------------------
ImoObj* ImFactory::inject(int type, Document* pDoc)
{
    ImoObj* pObj = NULL;
    switch(type)
    {
        case k_imo_attachments:         pObj = LOMSE_NEW ImoAttachments();        break;
        case k_imo_barline:             pObj = LOMSE_NEW ImoBarline();            break;
        case k_imo_beam:                pObj = LOMSE_NEW ImoBeam();               break;
        case k_imo_beam_dto:            pObj = LOMSE_NEW ImoBeamDto();            break;
        case k_imo_bezier_info:         pObj = LOMSE_NEW ImoBezierInfo();         break;
//        case k_imo_border_dto:      pObj = LOMSE_NEW ImoBorderDto();              break;
        case k_imo_textblock_info:      pObj = LOMSE_NEW ImoTextBlockInfo();      break;
        case k_imo_button:              pObj = LOMSE_NEW ImoButton();             break;
        case k_imo_chord:               pObj = LOMSE_NEW ImoChord();              break;
        case k_imo_clef:                pObj = LOMSE_NEW ImoClef();               break;
        case k_imo_color_dto:           pObj = LOMSE_NEW ImoColorDto();           break;
        case k_imo_content:             pObj = LOMSE_NEW ImoContent(pDoc);        break;
        case k_imo_cursor_info:         pObj = LOMSE_NEW ImoCursorInfo();         break;
        case k_imo_document:            pObj = LOMSE_NEW ImoDocument(pDoc);       break;
        case k_imo_dynamic:             pObj = LOMSE_NEW ImoDynamic();            break;
        case k_imo_fermata:             pObj = LOMSE_NEW ImoFermata();            break;
//        case k_imo_figured_bass:    pObj = LOMSE_NEW ImoFiguredBass();              break;
//        case k_imo_figured_bass_info:   pObj = LOMSE_NEW ImoFBInfo();          break;
        case k_imo_font_style_dto:      pObj = LOMSE_NEW ImoFontStyleDto();       break;
        case k_imo_go_back_fwd:         pObj = LOMSE_NEW ImoGoBackFwd();          break;
        case k_imo_heading:             pObj = LOMSE_NEW ImoHeading();            break;
        case k_imo_image:               pObj = LOMSE_NEW ImoImage();              break;
        case k_imo_inline_wrapper:      pObj = LOMSE_NEW ImoInlineWrapper();      break;
        case k_imo_instr_group:         pObj = LOMSE_NEW ImoInstrGroup();         break;
        case k_imo_instrument:          pObj = LOMSE_NEW ImoInstrument(pDoc);     break;
        case k_imo_instruments:         pObj = LOMSE_NEW ImoInstruments();        break;
        case k_imo_instrument_groups:   pObj = LOMSE_NEW ImoInstrGroups();        break;
        case k_imo_key_signature:       pObj = LOMSE_NEW ImoKeySignature();       break;
        case k_imo_line:                pObj = LOMSE_NEW ImoLine();               break;
        case k_imo_line_style:          pObj = LOMSE_NEW ImoLineStyle();          break;
        case k_imo_list:                pObj = LOMSE_NEW ImoList();               break;
        case k_imo_listitem:            pObj = LOMSE_NEW ImoListItem();           break;
        case k_imo_link:                pObj = LOMSE_NEW ImoLink();               break;
        case k_imo_metronome_mark:      pObj = LOMSE_NEW ImoMetronomeMark();      break;
        case k_imo_midi_info:           pObj = LOMSE_NEW ImoMidiInfo();           break;
        case k_imo_music_data:          pObj = LOMSE_NEW ImoMusicData();          break;
        case k_imo_note:                pObj = LOMSE_NEW ImoNote();               break;
        case k_imo_option:              pObj = LOMSE_NEW ImoOptionInfo();         break;
        case k_imo_options:             pObj = LOMSE_NEW ImoOptions();            break;
        case k_imo_page_info:           pObj = LOMSE_NEW ImoPageInfo();           break;
        case k_imo_para:                pObj = LOMSE_NEW ImoParagraph();          break;
        case k_imo_param_info:          pObj = LOMSE_NEW ImoParamInfo();          break;
//        case k_imo_point_dto:       pObj = LOMSE_NEW ImoParagraph();              break;
        case k_imo_reldataobjs:         pObj = LOMSE_NEW ImoReldataobjs();        break;
        case k_imo_rest:                pObj = LOMSE_NEW ImoRest();               break;
        case k_imo_score:               pObj = LOMSE_NEW ImoScore(pDoc);          break;
        case k_imo_score_text:          pObj = LOMSE_NEW ImoScoreText();          break;
        case k_imo_score_title:         pObj = LOMSE_NEW ImoScoreTitle();         break;
//        case k_imo_size_dto:        pObj = LOMSE_NEW ImoParagraph();              break;
        case k_imo_slur:                pObj = LOMSE_NEW ImoSlur();               break;
        case k_imo_slur_dto:            pObj = LOMSE_NEW ImoSlurDto();            break;
        case k_imo_spacer:              pObj = LOMSE_NEW ImoSpacer();             break;
        case k_imo_staff_info:          pObj = LOMSE_NEW ImoStaffInfo();          break;
        case k_imo_style:               pObj = LOMSE_NEW ImoStyle();              break;
        case k_imo_styles:              pObj = LOMSE_NEW ImoStyles(pDoc);         break;
        case k_imo_system_break:        pObj = LOMSE_NEW ImoSystemBreak();        break;
        case k_imo_system_info:         pObj = LOMSE_NEW ImoSystemInfo();         break;
        case k_imo_text_box:            pObj = LOMSE_NEW ImoTextBox();            break;
        case k_imo_text_info:           pObj = LOMSE_NEW ImoTextInfo();           break;
        case k_imo_text_item:           pObj = LOMSE_NEW ImoTextItem();           break;
//        case k_imo_text_style:      pObj = LOMSE_NEW ImoParagraph();              break;
        case k_imo_tie:                 pObj = LOMSE_NEW ImoTie();                break;
        case k_imo_tie_dto:             pObj = LOMSE_NEW ImoTieDto();             break;
        case k_imo_time_signature:      pObj = LOMSE_NEW ImoTimeSignature();      break;
        case k_imo_tuplet:              pObj = LOMSE_NEW ImoTuplet();             break;
        case k_imo_tuplet_dto:          pObj = LOMSE_NEW ImoTupletDto();          break;

        default:
            throw std::runtime_error("[ImFactory::inject] invalid type.");
    }
    if (!pObj->is_dto())
        pObj->set_id( pDoc->new_id() );
    return pObj;
}

//---------------------------------------------------------------------------------------
ImoBeamData* ImFactory::inject_beam_data(Document* pDoc, ImoBeamDto* pDto)
{
    ImoBeamData* pObj = LOMSE_NEW ImoBeamData(pDto);
    pObj->set_id( pDoc->new_id() );
    return pObj;
}

//---------------------------------------------------------------------------------------
ImoTieData* ImFactory::inject_tie_data(Document* pDoc, ImoTieDto* pDto)
{
    ImoTieData* pObj = LOMSE_NEW ImoTieData(pDto);
    pObj->set_id( pDoc->new_id() );
    return pObj;
}

//---------------------------------------------------------------------------------------
ImoTupletData* ImFactory::inject_tuplet_data(Document* pDoc, ImoTupletDto* pDto)
{
    ImoTupletData* pObj = LOMSE_NEW ImoTupletData(pDto);
    pObj->set_id( pDoc->new_id() );
    return pObj;
}

//---------------------------------------------------------------------------------------
ImoSlurData* ImFactory::inject_slur_data(Document* pDoc, ImoSlurDto* pDto)
{
    ImoSlurData* pObj = LOMSE_NEW ImoSlurData(pDto);
    pObj->set_id( pDoc->new_id() );
    return pObj;
}

//---------------------------------------------------------------------------------------
ImoTuplet* ImFactory::inject_tuplet(Document* pDoc, ImoTupletDto* pDto)
{
    ImoTuplet* pObj = LOMSE_NEW ImoTuplet(pDto);
    pObj->set_id( pDoc->new_id() );
    return pObj;
}

//---------------------------------------------------------------------------------------
ImoTextBox* ImFactory::inject_text_box(Document* pDoc, ImoTextBlockInfo& dto)
{
    ImoTextBox* pObj = LOMSE_NEW ImoTextBox(dto);
    pObj->set_id( pDoc->new_id() );
    return pObj;
}

//---------------------------------------------------------------------------------------
ImoNote* ImFactory::inject_note(Document* pDoc, int step, int octave,
                                int noteType, EAccidentals accidentals,
                                int dots, int staff, int voice, int stem)
{
    ImoNote* pObj = LOMSE_NEW ImoNote(step, octave, noteType, accidentals, dots,
                                staff, voice, stem);
    pObj->set_id( pDoc->new_id() );
    return pObj;
}

//---------------------------------------------------------------------------------------
ImoMultiColumn* ImFactory::inject_multicolumn(Document* pDoc)
{
    ImoMultiColumn* pObj = LOMSE_NEW ImoMultiColumn();
    pObj->set_id( pDoc->new_id() );
    return pObj;
}

//---------------------------------------------------------------------------------------
ImoImage* ImFactory::inject_image(Document* pDoc, unsigned char* imgbuf, VSize bmpSize,
                                  EPixelFormat format, USize imgSize)
{
    ImoImage* pObj = LOMSE_NEW ImoImage(imgbuf, bmpSize, format, imgSize);
    pObj->set_id( pDoc->new_id() );
    return pObj;
}

//---------------------------------------------------------------------------------------
ImoControl* ImFactory::inject_control(Document* pDoc, Control* ctrol)
{
    ImoControl* pObj = LOMSE_NEW ImoControl(ctrol);
    pObj->set_id( pDoc->new_id() );
    return pObj;
}


}  //namespace lomse