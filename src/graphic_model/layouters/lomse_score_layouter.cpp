//---------------------------------------------------------------------------------------
// This file is part of the Lomse library.
// Lomse is copyrighted work (c) 2010-2016. All rights reserved.
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

#include "lomse_score_layouter.h"

#include "lomse_staffobjs_table.h"
#include "lomse_score_meter.h"
#include "lomse_calligrapher.h"
#include "lomse_graphical_model.h"
#include "lomse_gm_basic.h"
#include "lomse_internal_model.h"
#include "lomse_im_note.h"
#include "lomse_box_system.h"
#include "lomse_box_slice.h"
#include "lomse_box_slice_instr.h"
#include "lomse_shape_staff.h"
#include "lomse_shapes.h"
#include "lomse_shape_note.h"
#include "lomse_shape_text.h"
#include "lomse_engraving_options.h"
#include "lomse_system_layouter.h"
#include "lomse_staffobjs_cursor.h"
#include "lomse_shape_barline.h"
#include "lomse_shape_line.h"
#include "lomse_barline_engraver.h"
#include "lomse_beam_engraver.h"
#include "lomse_chord_engraver.h"
#include "lomse_clef_engraver.h"
#include "lomse_fermata_engraver.h"
#include "lomse_instrument_engraver.h"
#include "lomse_key_engraver.h"
#include "lomse_line_engraver.h"
#include "lomse_metronome_engraver.h"
#include "lomse_note_engraver.h"
#include "lomse_rest_engraver.h"
#include "lomse_slur_engraver.h"
#include "lomse_text_engraver.h"
#include "lomse_tie_engraver.h"
#include "lomse_time_engraver.h"
#include "lomse_tuplet_engraver.h"
#include "lomse_articulation_engraver.h"
#include "lomse_dynamics_mark_engraver.h"
#include "lomse_ornament_engraver.h"
#include "lomse_technical_engraver.h"
#include "lomse_lyric_engraver.h"
#include "lomse_spacing_algorithm_gourlay.h"

namespace lomse
{


//=======================================================================================
// ScoreLayouter implementation
//=======================================================================================
ScoreLayouter::ScoreLayouter(ImoContentObj* pItem, Layouter* pParent,
                             GraphicModel* pGModel, LibraryScope& libraryScope)
    : Layouter(pItem, pParent, pGModel, libraryScope, NULL, true)
    , m_libraryScope(libraryScope)
    , m_pScore( dynamic_cast<ImoScore*>(pItem) )
    , m_pScoreMeter( LOMSE_NEW ScoreMeter(m_pScore) )
    , m_pSpAlgorithm(NULL)
    , m_pShapesCreator(NULL)
    , m_pPartsEngraver(NULL)
    , m_pStub(NULL)
    , m_pCurBoxPage(NULL)
    , m_pCurBoxSystem(NULL)
    , m_iColumnToTrace(-1)
    , m_nTraceLevel(k_trace_off)
{
}

//---------------------------------------------------------------------------------------
ScoreLayouter::~ScoreLayouter()
{
    delete m_pPartsEngraver;
    delete_system_layouters();
    delete m_pScoreMeter;
    delete m_pSpAlgorithm;
    delete m_pShapesCreator;
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::prepare_to_start_layout()
{
    //initialize base class
    Layouter::prepare_to_start_layout();

    //Create all auxiliary helper objects (PartsEngraver, ShapesCreator,
    //ColumnsBuilder), and initialize internal variables
    initialice_score_layouter();

    //start the real work by asking PartsEngraver to decide systems indentation
    //In this step, GroupEngraver object measures the group brace/bracket, the group
    //name and the instruments names/abbrevs, as well as possible braces or brackets
    //for multi-staff instruments.
    //As part of the process, instrument engravers assign height for system and
    //position for staves.
    decide_systems_indentation();

    //Next the score is split in columns (small chunks, e.g. measures) and
    //the spacing algorithm is applied
    m_pSpAlgorithm->split_content_in_columns();
    m_pSpAlgorithm->do_spacing_algorithm();
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::layout_in_box()
{
    LOMSE_LOG_DEBUG(Logger::k_layout, "");

    //AWARE: This method is invoked to layout a page. If there are more pages to
    //layout, it will be invoked more times. Therefore, this method must not initialize
    //anything. All initializations must be done in 'prepare_to_start_layout()'.
    //layout_in_box() method must always continue laying out from current state.

    //First, information about the page to layout is obtained (e.g. page size) and
    //cursor is moved to top of page box.
    page_initializations(m_pItemMainBox);
    move_cursor_to_top_left_corner();


    if (is_first_page())
    {
        decide_line_breaks();
        //AWARE: deciding line breaks cannot be moved to the preparation phase because
        //for deciding break points it is necessary to know page size, and this
        //information is not known in the preparation phase.

        add_score_titles();
    }


    //loop for creating and adding systems
    bool fSystemsAdded = false;
    while(m_iCurColumn < get_num_columns() || system_created())
    {
        if (!system_created())
            create_system();

        if (enough_space_in_page_for_system())
        {
            add_system_to_page();
            fSystemsAdded = true;
        }

        else if (!fSystemsAdded && score_page_is_the_only_content_of_parent_box())
        {
            //TODO: ScoreLayouter::layout_in_box()
            //  If paper height is smaller than system height it is impossible to fit
            //  one system in a page. We have to split system horizontally (some staves in
            //  one page and the others in next page). But this is not yet implemented.
            //  Therefore, for now, just an error message.
            add_error_message("ERROR: Not enough space for drawing just one system!");
            stringstream msg;
            msg << "  Page size too small for " << m_pScore->get_num_instruments()
                << " instruments.";
            add_error_message(msg.str());
            set_layout_is_finished(true);
            delete_system();
            return;
        }
        else
        {
            //inform parent layouter for allocating a new page.
            set_layout_is_finished(false);
            return;
        }
    }


    //add empty systems to fill the page, if option enabled, and
    //remove unused space
    bool fMoreColumns = m_iCurColumn < get_num_columns();
    if (!fMoreColumns)
    {
        fill_page_with_empty_systems_if_required();
        remove_unused_space();
    }

    set_layout_is_finished( !fMoreColumns );
    m_pageCursor = m_cursor;
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::initialice_score_layouter()
{
    create_parts_engraver();

    m_pShapesCreator = LOMSE_NEW ShapesCreator(m_libraryScope, m_pScoreMeter,
                                               m_shapesStorage, m_pPartsEngraver);

    m_pSpAlgorithm = LOMSE_NEW SpAlgGourlay(m_libraryScope, m_pScoreMeter,
                                            this, m_pScore, m_shapesStorage,
                                            m_pShapesCreator, m_pPartsEngraver);

    get_score_renderization_options();

    //For debugging:
    //ColStaffObjs* pCol = m_pScore->get_staffobjs_table();
    //cout << pCol->dump()  << endl;

	m_iCurPage = -1;
    m_iCurColumn = -1;
    m_iCurSystem = -1;
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::create_system()
{
    create_system_layouter();
    create_system_box();
    engrave_system();
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::remove_unused_space()
{
    //adjust height of last page (remove unused space)

    LUnits yBottom = m_pCurSysLyt->get_y_max();
    ImoStyle* pStyle = m_pScore->get_style();
    if (pStyle)
    {
        yBottom += pStyle->margin_bottom();
        yBottom += pStyle->border_width_bottom();
        yBottom += pStyle->padding_bottom();
    }
    m_pCurBoxPage->set_height( yBottom - m_pCurBoxPage->get_top());
    m_cursor.y = m_pCurBoxPage->get_bottom();
}

//---------------------------------------------------------------------------------------
bool ScoreLayouter::system_created()
{
    return m_pCurBoxSystem != NULL;
}

//---------------------------------------------------------------------------------------
bool ScoreLayouter::enough_space_in_page_for_system()
{
    LUnits height = m_pCurBoxSystem->get_height();
    return remaining_height() >= height;
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::delete_system()
{
    delete m_pCurBoxSystem;
    m_pCurBoxSystem = NULL;
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::create_system_layouter()
{
    m_pCurSysLyt = LOMSE_NEW SystemLayouter(this, m_libraryScope, m_pScoreMeter,
                                      m_pScore, m_shapesStorage, m_pShapesCreator,
                                      m_pPartsEngraver, m_pSpAlgorithm);
    m_sysLayouters.push_back(m_pCurSysLyt);
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::engrave_system()
{
    LUnits indent = get_system_indent();
    if (get_num_columns() == 0)
    {
        //force to layout an empty system
        m_pCurSysLyt->engrave_system(indent, 0, 0, m_cursor);
    }
    else
    {
        int iFirstCol = m_breaks[m_iCurSystem];
        int iLastCol = (m_iCurSystem == get_num_systems() - 1 ?
                                        get_num_columns() : m_breaks[m_iCurSystem + 1] );
        m_pCurSysLyt->engrave_system(indent, iFirstCol, iLastCol, m_cursor);
    }
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::create_system_box()
{
    m_iCurSystem++;

    LUnits width = m_pCurBoxPage->get_width();
    LUnits top = m_cursor.y
                 + distance_to_top_of_system(m_iCurSystem, m_fFirstSystemInPage);
    LUnits left = m_cursor.x;

    //save info for repositioning system if necessary
    m_iSysPage = m_iCurPage;
    m_sysCursor = m_cursor;

    ImoSystemInfo* pInfo = m_pScore->get_other_system_info();

    LUnits height = determine_system_top_margin();      //top margin
    height += m_pSpAlgorithm->get_staves_height();      //staves height
    height += pInfo->get_system_distance() / 2.0f;      //bottom margin

    m_pCurBoxSystem = m_pCurSysLyt->create_system_box(left, top, width, height);
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::add_system_to_page()
{
    reposition_system_if_page_has_changed();

    m_pCurBoxPage->add_system(m_pCurBoxSystem, m_iCurSystem);
    m_pCurBoxSystem->add_shapes_to_tables();

    move_paper_cursor_to_bottom_of_added_system();
    is_first_system_in_page(false);
    m_pCurBoxSystem = NULL;
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::reposition_system_if_page_has_changed()
{
    //if the parent box used when engraving the system has changed (because not
    //enough space in parent box to add the system and a new page has been added)
    //it is necessary to reposition all content of the engraved system.

    if (m_iSysPage != m_iCurPage)
    {
        USize shift(m_cursor.x - m_sysCursor.x,
                    m_cursor.y - m_sysCursor.y );
        m_pCurBoxSystem->shift_origin_and_content(shift);
        m_pCurSysLyt->on_origin_shift(shift.height);
    }
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::move_paper_cursor_to_bottom_of_added_system()
{
    m_cursor.x = m_pCurBoxPage->get_left();
    m_cursor.y = m_pCurBoxSystem->get_bottom();
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::decide_systems_indentation()
{
    m_pPartsEngraver->decide_systems_indentation();

    m_uFirstSystemIndent = m_pPartsEngraver->get_first_system_indent();
    m_uOtherSystemIndent = m_pPartsEngraver->get_other_system_indent();
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::page_initializations(GmoBox* pContainerBox)
{
    m_iCurPage++;
    m_pCurBoxPage = dynamic_cast<GmoBoxScorePage*>( pContainerBox );
    is_first_system_in_page(true);
    m_startTop = m_pCurBoxPage->get_top();
}

//---------------------------------------------------------------------------------------
LUnits ScoreLayouter::get_first_system_staves_size()
{
    ImoSystemInfo* pInfo = m_pScore->get_first_system_info();
    LUnits leftMargin = pInfo->get_left_margin();
    LUnits rightMargin = pInfo->get_right_margin();
    return m_pCurBoxPage->get_width() - leftMargin - rightMargin - m_uFirstSystemIndent;
}

//---------------------------------------------------------------------------------------
LUnits ScoreLayouter::get_other_systems_staves_size()
{
    ImoSystemInfo* pInfo = m_pScore->get_other_system_info();
    LUnits leftMargin = pInfo->get_left_margin();
    LUnits rightMargin = pInfo->get_right_margin();
    return m_pCurBoxPage->get_width() - leftMargin - rightMargin - m_uOtherSystemIndent;
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::decide_line_breaks()
{
    if (get_num_columns() != 0)
    {
        LinesBreakerOptimal breaker(this, m_pSpAlgorithm, m_breaks);
        breaker.decide_line_breaks();
        //breaker.dump_entries();
    }
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::create_main_box(GmoBox* pParentBox, UPoint pos, LUnits width,
                                    LUnits height)
{
    m_pItemMainBox = LOMSE_NEW GmoBoxScorePage(m_pScore);
    pParentBox->add_child_box(m_pItemMainBox);
    create_stub();
    m_pStub->add_page( static_cast<GmoBoxScorePage*>(m_pItemMainBox) );

    m_pItemMainBox->set_origin(pos);
    m_pItemMainBox->set_width(width);
    m_pItemMainBox->set_height(height);
}

//---------------------------------------------------------------------------------------
bool ScoreLayouter::enough_space_for_empty_system()
{
    LUnits height = m_pSpAlgorithm->get_staves_height();
    height += distance_to_top_of_system(m_iCurSystem+1, m_fFirstSystemInPage);
    height += determine_system_top_margin();
    return remaining_height() >= height;
}

//---------------------------------------------------------------------------------------
LUnits ScoreLayouter::remaining_height()
{
    return m_pCurBoxPage->get_height() - (m_cursor.y - m_startTop);
}

//---------------------------------------------------------------------------------------
bool ScoreLayouter::score_page_is_the_only_content_of_parent_box()
{
    GmoBox* pParent = m_pCurBoxPage->get_parent_box();
    return m_pCurBoxPage->get_height() == pParent->get_height();
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::move_cursor_to_top_left_corner()
{
    m_cursor.x = m_pCurBoxPage->get_left();
    m_cursor.y = m_pCurBoxPage->get_top();
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::create_stub()
{
    m_pStub = m_pGModel->add_stub_for(m_pScore->get_id());
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::add_score_titles()
{
    list<ImoScoreTitle*>& titles = m_pScore->get_titles();
    list<ImoScoreTitle*>::iterator it;
    for (it = titles.begin(); it != titles.end(); ++it)
    {
        ImoScoreTitle* pImo = *it;
        TextEngraver engrv(m_libraryScope, m_pScoreMeter, pImo->get_text(),
                           pImo->get_language(), pImo->get_style());
        GmoShape* pShape = engrv.create_shape(pImo, m_cursor.x, m_cursor.y);
        m_pCurBoxPage->add_shape(pShape, GmoShape::k_layer_aux_objs);
        m_cursor.y += pShape->get_height();
    }
}

//---------------------------------------------------------------------------------------
LUnits ScoreLayouter::get_target_size_for_system(int iSystem)
{
    LUnits width = (iSystem == 0 ? get_first_system_staves_size()
                                 : get_other_systems_staves_size() );
    if (iSystem > 0)
        width -= space_used_by_prolog(iSystem);
    return width;
}

//---------------------------------------------------------------------------------------
LUnits ScoreLayouter::space_used_by_prolog(int UNUSED(iSystem))
{
    //TODO. Prolog width should be computed on each column.
    //For now, an estimation: the height of ten lines (two staff)
    return m_pScoreMeter->tenths_to_logical(100.0f, 0, 0);
}

//---------------------------------------------------------------------------------------
LUnits ScoreLayouter::distance_to_top_of_system(int iSystem, bool fFirstInPage)
{
    if (iSystem == 0)   //is_first_system_in_score())
    {
        ImoSystemInfo* pInfo = m_pScore->get_first_system_info();
        return pInfo->get_top_system_distance() - pInfo->get_system_distance() / 2.0f;
    }
    else if (fFirstInPage)  //is_first_system_in_page())
    {
        ImoSystemInfo* pInfo = m_pScore->get_other_system_info();
        return pInfo->get_top_system_distance() - pInfo->get_system_distance() / 2.0f;
    }
    else
        return 0.0f;
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::determine_staff_lines_horizontal_position(int iInstr)
{
    LUnits indent = get_system_indent();
    LUnits width = m_pCurBoxSystem->get_content_width_old();
    m_pPartsEngraver->set_staves_horizontal_position(iInstr, m_cursor.x, width, indent);
    m_cursor.x += indent;
}

//---------------------------------------------------------------------------------------
LUnits ScoreLayouter::determine_top_space(int iInstr, bool fFirstSystemInScore,
                                          bool fFirstSystemInPage)
{
    if (iInstr == 0)
    {
        //distance_to_first_staff
        if (fFirstSystemInScore)
        {
            ImoSystemInfo* pInfo = m_pScore->get_first_system_info();
            return pInfo->get_system_distance() / 2.0f;
        }
        else if (fFirstSystemInPage)
        {
            ImoSystemInfo* pInfo = m_pScore->get_other_system_info();
            return pInfo->get_system_distance() / 2.0f;
        }
        else
        {
            ImoSystemInfo* pInfo = m_pScore->get_other_system_info();
            return pInfo->get_system_distance() / 2.0f;
        }
    }
    else
    {
        ImoInstrument* pInstr = m_pScore->get_instrument(iInstr);
        return pInstr->get_staff(0)->get_staff_margin() / 2.0f;
    }
}

//---------------------------------------------------------------------------------------
LUnits ScoreLayouter::determine_system_top_margin()
{
    if (is_first_system_in_score())
    {
        ImoSystemInfo* pInfo = m_pScore->get_first_system_info();
        return pInfo->get_system_distance() / 2.0f;
    }
    else if (is_first_system_in_page())
    {
        ImoSystemInfo* pInfo = m_pScore->get_other_system_info();
        return pInfo->get_system_distance() / 2.0f;
    }
    else
    {
        ImoSystemInfo* pInfo = m_pScore->get_other_system_info();
        return pInfo->get_system_distance() / 2.0f;
    }
}

//---------------------------------------------------------------------------------------
int ScoreLayouter::get_system_containing_column(int iCol)
{
    if (iCol > 0)
    {

        int maxSystem = get_num_systems() - 1;
        for (int iSys = 0; iSys < maxSystem; ++iSys)
        {
            if (iCol >= m_breaks[iSys] && iCol < m_breaks[iSys+1])
                return iSys;
        }
        return maxSystem;
    }
    else
        return 0;
}

//---------------------------------------------------------------------------------------
bool ScoreLayouter::is_system_empty(int iSystem)
{
    if (get_num_systems() == 0 || iSystem >= get_num_systems())
        return true;

    int iFirtsCol = m_breaks[iSystem];
    return m_pSpAlgorithm->is_empty_column(iFirtsCol);
}

//---------------------------------------------------------------------------------------
int ScoreLayouter::get_num_columns()
{
    return m_pSpAlgorithm->get_num_columns();
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::create_parts_engraver()
{
    ImoInstrGroups* pGroups = m_pScore->get_instrument_groups();
    m_pPartsEngraver = LOMSE_NEW PartsEngraver(m_libraryScope, m_pScoreMeter,
                                               pGroups, m_pScore, this);
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::get_score_renderization_options()
{
    ImoOptionInfo* pOpt = m_pScore->get_option("StaffLines.StopAtFinalBarline");
    m_fStopStaffLinesAtFinalBarline = pOpt->get_bool_value();

    pOpt = m_pScore->get_option("Score.JustifyFinalBarline");
    m_fJustifyFinalBarline = (pOpt != NULL && pOpt->get_bool_value());
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::delete_system_layouters()
{
    std::vector<SystemLayouter*>::iterator it;
    for (it = m_sysLayouters.begin(); it != m_sysLayouters.end(); ++it)
        delete *it;
    m_sysLayouters.clear();
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::delete_not_used_objects()
{
    //Sanitizing method for unit tests. When the score layout process is not finished,
    //it is necessary to delete objects that, in normal processing, will be deleted
    //in other places

    //pendig aux objects
    std::list<PendingAuxObjs*>::iterator itPAO;
    for (itPAO = m_pendingAuxObjs.begin(); itPAO != m_pendingAuxObjs.end(); ++itPAO)
        delete *itPAO;
    m_pendingAuxObjs.clear();

    //not used shapes
    for (int iCol = 0; iCol < get_num_columns(); ++iCol)
        //m_pSpAlgorithm->delete_box_and_shapes(iCol, &m_shapesStorage);
        m_pSpAlgorithm->delete_box_and_shapes(iCol);
    m_shapesStorage.delete_ready_shapes();


    //not used engravers
    m_shapesStorage.delete_engravers();

    //system boxes
    std::vector<SystemLayouter*>::iterator it;
    for (it = m_sysLayouters.begin(); it != m_sysLayouters.end(); ++it)
    {
        delete (*it)->get_box_system();
    }
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::fill_page_with_empty_systems_if_required()
{
    if (!m_fStopStaffLinesAtFinalBarline)
    {
        ImoOptionInfo* pOpt = m_pScore->get_option("Score.FillPageWithEmptyStaves");
        bool fFillPage = pOpt->get_bool_value();
        if (fFillPage)
        {
            while(enough_space_for_empty_system())
            {
                create_empty_system();
                add_system_to_page();
            }
        }
    }
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::create_empty_system()
{
    create_system_layouter();
    create_system_box();
    engrave_empty_system();
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::engrave_empty_system()
{
    LUnits indent = get_system_indent();
    m_pCurSysLyt->engrave_system(indent, 0, 0, m_cursor);
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::dump_column_data(int iCol, ostream& outStream)
{
    m_pSpAlgorithm->dump_column_data(iCol, outStream);
}

//---------------------------------------------------------------------------------------
LUnits ScoreLayouter::get_column_width(int iCol)
{
    return m_pSpAlgorithm->get_column_width(iCol);
}

//---------------------------------------------------------------------------------------
bool ScoreLayouter::column_has_system_break(int iCol)
{
    return m_pSpAlgorithm->has_system_break(iCol);
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::add_error_message(const string& msg)
{
    ImoStyle* pStyle = m_pScore->get_default_style();
    TextEngraver engrv(m_libraryScope, m_pScoreMeter, msg, "en", pStyle);
    LUnits x = m_pageCursor.x + 400.0;
    LUnits y = m_pageCursor.y + 800.0;
    GmoShape* pText = engrv.create_shape(NULL, x, y);
    m_pItemMainBox->add_shape(pText, GmoShape::k_layer_top);
    m_pageCursor.y += pText->get_height();
}

//---------------------------------------------------------------------------------------
void ScoreLayouter::trace_column(int iCol, int level)
{
    m_iColumnToTrace = iCol;
    m_nTraceLevel = level;
    m_pSpAlgorithm->set_trace_level(iCol, level);
}



//=======================================================================================
// ColumnBreaker implementation
//=======================================================================================
ColumnBreaker::ColumnBreaker(int numInstruments, StaffObjsCursor* pSysCursor)
    : m_numInstruments(numInstruments)
    , m_consecutiveBarlines(0)
    , m_numInstrWithTS(0)
    , m_targetBreakTime(0.0f)
    , m_lastBarlineTime(0.0f)
    , m_maxMeasureDuration(0.0f)
    , m_lastBreakTime(0.0f)
{
    m_numLines = pSysCursor->get_num_lines();
    m_measures.reserve(numInstruments);
    m_measures.assign(numInstruments, 0.0f);
    m_beamed.reserve(m_numLines);
    m_beamed.assign(m_numLines, false);
    m_tied.reserve(m_numLines);
    m_tied.assign(m_numLines, false);
}

//---------------------------------------------------------------------------------------
bool ColumnBreaker::feasible_break_before_this_obj(ImoStaffObj* pSO, TimeUnits rTime,
                                                   int iInstr, int iLine)
{
    bool fBreak = false;
    if (!pSO->is_barline()
        && m_consecutiveBarlines > 0
        && m_consecutiveBarlines >= m_numInstrWithTS
       )
    {
        fBreak = true;
    }
    else if (pSO->is_note_rest()
             && rTime > m_lastBreakTime
             && rTime > m_lastBarlineTime + m_maxMeasureDuration
            )
    {
        fBreak = is_suitable_note_rest(pSO, rTime);
    }

    //save data
    if (pSO->is_note_rest())
    {
        ImoNoteRest* pNR = static_cast<ImoNoteRest*>(pSO);
        m_beamed[iLine] = pNR->is_beamed() && !pNR->is_end_of_beam();

        if (pSO->is_note())
            m_tied[iLine] = static_cast<ImoNote*>(pSO)->is_tied_next();
        else
            m_tied[iLine] = false;

        TimeUnits rNextTime = rTime + pNR->get_duration();
        m_targetBreakTime = max(m_targetBreakTime, rNextTime);
    }

    if (pSO->is_time_signature())
    {
        ImoTimeSignature* pTS = static_cast<ImoTimeSignature*>(pSO);
        m_measures[iInstr] = pTS->get_measure_duration();
        m_maxMeasureDuration = 0.0f;
        m_numInstrWithTS = 0;
        for (int i=0; i < m_numInstruments; ++i)
        {
            m_maxMeasureDuration = max(m_maxMeasureDuration, m_measures[i]);
            m_numInstrWithTS += (m_measures[i] > 0.0f ? 1 : 0);
        }
    }

    if (pSO->is_barline())
    {
        if (!static_cast<ImoBarline*>(pSO)->is_middle())
        {
            m_lastBarlineTime = rTime;
            ++m_consecutiveBarlines;
        }
    }
    else
        m_consecutiveBarlines = 0;

    //if suitable point, save and clear data
    if (fBreak)
    {
        m_lastBreakTime = rTime;
        m_consecutiveBarlines = 0;
    }

    return fBreak;
}

//---------------------------------------------------------------------------------------
bool ColumnBreaker::is_suitable_note_rest(ImoStaffObj* pSO, TimeUnits rTime)
{
    if (pSO->is_note_rest())
    {
        bool fBreak = true;      //assume it is a suitable point

        //not suitable if breaks a beam or a tie
        for (int i=0; i < m_numLines; ++i)
        {
            fBreak &= !m_beamed[i];
            fBreak &= !m_tied[i];
        }

        //not suitable if is tied to prev note
        if (pSO->is_note())
            fBreak &= !static_cast<ImoNote*>(pSO)->is_tied_prev();

        //not suitable if next note is within a previous voice duration
        fBreak &= !is_lower_time(rTime, m_targetBreakTime);

        return fBreak;
    }
    return false;
}



//=======================================================================================
// ShapesCreator implementation
//=======================================================================================

//---------------------------------------------------------------------------------------
// helper engraver to create an invisible shape
class InvisibleEngraver : public Engraver
{
public:
    InvisibleEngraver(LibraryScope& libraryScope, ScoreMeter* pScoreMeter)
        : Engraver(libraryScope, pScoreMeter)
    {
    }

    GmoShape* create_shape(ImoObj* pCreatorImo, ShapeId idx, UPoint uPos, USize uSize)
    {
        return LOMSE_NEW GmoShapeInvisible(pCreatorImo, idx, uPos, uSize);
    }
};


//---------------------------------------------------------------------------------------
ShapesCreator::ShapesCreator(LibraryScope& libraryScope, ScoreMeter* pScoreMeter,
                             ShapesStorage& shapesStorage, PartsEngraver* pPartsEngraver)
    : m_libraryScope(libraryScope)
    , m_pScoreMeter(pScoreMeter)
    , m_shapesStorage(shapesStorage)
    , m_pPartsEngraver(pPartsEngraver)
{
}

//---------------------------------------------------------------------------------------
GmoShape* ShapesCreator::create_staffobj_shape(ImoStaffObj* pSO, int iInstr, int iStaff,
                                               UPoint pos, int clefType, unsigned flags)
{
    //factory method to create shapes for staffobjs

    if (!pSO->is_visible())
        return create_invisible_shape(pSO, iInstr, iStaff, pos, 0.0f);

    switch (pSO->get_obj_type())
    {
        case k_imo_barline:
        {
            ImoBarline* pImo = static_cast<ImoBarline*>(pSO);
            InstrumentEngraver* pInstrEngrv =
                m_pPartsEngraver->get_engraver_for(iInstr);
            LUnits yTop = pInstrEngrv->get_barline_top();
            LUnits yBottom = pInstrEngrv->get_barline_bottom();
            BarlineEngraver engrv(m_libraryScope, m_pScoreMeter, iInstr);
            Color color = pImo->get_color();
            return engrv.create_shape(pImo, pos.x, yTop, yBottom, color);
        }
        case k_imo_clef:
        {
            bool fSmallClef = flags && k_flag_small_clef;
            ImoClef* pClef = static_cast<ImoClef*>(pSO);
            int clefSize = pClef->get_symbol_size();
            if (clefSize == k_size_default)
                clefSize = fSmallClef ? k_size_cue : k_size_full;
            Color color = pClef->get_color();
            ClefEngraver engrv(m_libraryScope, m_pScoreMeter, iInstr, iStaff);
            return engrv.create_shape(pClef, pos, clefType, clefSize, color);
        }
        case k_imo_key_signature:
        {
            ImoKeySignature* pImo = static_cast<ImoKeySignature*>(pSO);
            KeyEngraver engrv(m_libraryScope, m_pScoreMeter, iInstr, iStaff);
            Color color = pImo->get_color();
            return engrv.create_shape(pImo, clefType, pos, color);
        }
        case k_imo_note:
        {
            ImoNote* pImo = static_cast<ImoNote*>(pSO);
            NoteEngraver engrv(m_libraryScope, m_pScoreMeter, &m_shapesStorage,
                               iInstr, iStaff);
            Color color = pImo->get_color();
            GmoShape* pShape = engrv.create_shape(pImo, clefType, pos, color);

            //AWARE: Chords are an exception to the way relations are engraved. This
            //is because chords affect to note positions (reverse noteheads, shift
            //accidentals) and this, in turn, affects column/system laying out.
            //Therefore, I decided to add an exception and force early layout of chord.
            engrv.add_to_chord_if_in_chord();

            return pShape;
        }
        case k_imo_rest:
        {
            ImoRest* pImo = static_cast<ImoRest*>(pSO);
            if (pImo->is_go_fwd())
            {
                return create_invisible_shape(pSO, iInstr, iStaff, pos, 0.0f);
            }
            else
            {
                RestEngraver engrv(m_libraryScope, m_pScoreMeter, &m_shapesStorage,
                                   iInstr, iStaff);
                Color color = pImo->get_color();
                return engrv.create_shape(pImo, pos, color);
            }
        }
        case k_imo_time_signature:
        {
            ImoTimeSignature* pImo = static_cast<ImoTimeSignature*>(pSO);
            TimeEngraver engrv(m_libraryScope, m_pScoreMeter, iInstr, iStaff);
            Color color = pImo->get_color();
            return engrv.create_shape(pImo, pos, color);
        }
        case k_imo_spacer:
        {
            ImoSpacer* pImo = static_cast<ImoSpacer*>(pSO);
            LUnits space = m_pScoreMeter->tenths_to_logical(pImo->get_width(),
                                                            iInstr, iStaff);
            return create_invisible_shape(pSO, iInstr, iStaff, pos, space);
        }
        case k_imo_metronome_mark:
        {
            ImoMetronomeMark* pImo = static_cast<ImoMetronomeMark*>(pSO);
            MetronomeMarkEngraver engrv(m_libraryScope, m_pScoreMeter, iInstr, iStaff);
            Color color = pImo->get_color();
            return engrv.create_shape(pImo, pos, color);
        }
        default:
            return create_invisible_shape(pSO, iInstr, iStaff, pos, 0.0f);
    }
}

//---------------------------------------------------------------------------------------
GmoShape* ShapesCreator::create_auxobj_shape(ImoAuxObj* pAO, int iInstr, int iStaff,
                                             GmoShape* pParentShape)
{
    //factory method to create shapes for auxobjs

    InstrumentEngraver* pInstrEngrv = m_pPartsEngraver->get_engraver_for(iInstr);
    LUnits yTop = pInstrEngrv->get_top_line_of_staff(iStaff);

    UPoint pos((pParentShape->get_left() + pParentShape->get_width() / 2.0f), yTop);
    switch (pAO->get_obj_type())
    {
        case k_imo_articulation_line:
        case k_imo_articulation_symbol:
        {
            ImoArticulation* pImo = static_cast<ImoArticulation*>(pAO);
            ArticulationEngraver engrv(m_libraryScope, m_pScoreMeter, iInstr, iStaff);
            Color color = pImo->get_color();
            return engrv.create_shape(pImo, pos, color, pParentShape);
        }
        case k_imo_dynamics_mark:
        {
            ImoDynamicsMark* pImo = static_cast<ImoDynamicsMark*>(pAO);
            DynamicsMarkEngraver engrv(m_libraryScope, m_pScoreMeter, iInstr, iStaff);
            Color color = pImo->get_color();
            return engrv.create_shape(pImo, pos, color, pParentShape);
        }
        case k_imo_fermata:
        {
            ImoFermata* pImo = static_cast<ImoFermata*>(pAO);
            FermataEngraver engrv(m_libraryScope, m_pScoreMeter, iInstr, iStaff);
            Color color = pImo->get_color();
            return engrv.create_shape(pImo, pos, color, pParentShape);
        }
        case k_imo_ornament:
        {
            ImoOrnament* pImo = static_cast<ImoOrnament*>(pAO);
            OrnamentEngraver engrv(m_libraryScope, m_pScoreMeter, iInstr, iStaff);
            Color color = pImo->get_color();
            return engrv.create_shape(pImo, pos, color, pParentShape);
        }
        case k_imo_score_line:
        {
            ImoScoreLine* pImo = static_cast<ImoScoreLine*>(pAO);
            LineEngraver engrv(m_libraryScope, m_pScoreMeter, iInstr, iStaff);
            return engrv.create_shape(pImo, pos);
        }
        case k_imo_score_text:
        {
            ImoScoreText* pImo = static_cast<ImoScoreText*>(pAO);
            TextEngraver engrv(m_libraryScope, m_pScoreMeter, pImo->get_text(),
                               pImo->get_language(), pImo->get_style());
            return engrv.create_shape(pImo, pos.x, pos.y);
        }
        case k_imo_technical:
        {
            ImoTechnical* pImo = static_cast<ImoTechnical*>(pAO);
            TechnicalEngraver engrv(m_libraryScope, m_pScoreMeter, iInstr, iStaff);
            Color color = pImo->get_color();
            return engrv.create_shape(pImo, pos, color, pParentShape);
        }
        default:
            return create_invisible_shape(pAO, iInstr, iStaff, pos, 0.0f);
    }
}

//---------------------------------------------------------------------------------------
GmoShape* ShapesCreator::create_invisible_shape(ImoObj* pSO, int iInstr, int iStaff,
                                                UPoint uPos, LUnits width)
{
    InvisibleEngraver engrv(m_libraryScope, m_pScoreMeter);
    USize uSize(width, m_pScoreMeter->tenths_to_logical(40.0f, iInstr, iStaff));
    return engrv.create_shape(pSO, 0, uPos, uSize);
}

//---------------------------------------------------------------------------------------
void ShapesCreator::start_engraving_relobj(ImoRelObj* pRO,
                                           ImoStaffObj* pSO,
                                           GmoShape* pStaffObjShape,
                                           int iInstr, int iStaff, int iSystem,
                                           int iCol, int UNUSED(iLine),
                                           ImoInstrument* UNUSED(pInstr))
{
    //factory method to create the engraver for relation auxobjs

    RelObjEngraver* pEngrv = NULL;
    switch (pRO->get_obj_type())
    {
        case k_imo_beam:
        {
            pEngrv = LOMSE_NEW BeamEngraver(m_libraryScope, m_pScoreMeter);
            break;
        }

        case k_imo_slur:
        {
            InstrumentEngraver* pInstrEngrv = m_pPartsEngraver->get_engraver_for(iInstr);
            pEngrv = LOMSE_NEW SlurEngraver(m_libraryScope, m_pScoreMeter, pInstrEngrv);
            break;
        }

        case k_imo_tie:
        {
            InstrumentEngraver* pInstrEngrv = m_pPartsEngraver->get_engraver_for(iInstr);
            LUnits xRight = pInstrEngrv->get_staves_right();
            LUnits xLeft = pInstrEngrv->get_staves_left();
            pEngrv = LOMSE_NEW TieEngraver(m_libraryScope, m_pScoreMeter, xLeft, xRight);
            break;
        }

        case k_imo_tuplet:
        {
            pEngrv = LOMSE_NEW TupletEngraver(m_libraryScope, m_pScoreMeter);
            break;
        }

        default:
            return;
    }

    if (pEngrv)
    {
        InstrumentEngraver* pInstrEngrv = m_pPartsEngraver->get_engraver_for(iInstr);
        LUnits xRight = pInstrEngrv->get_staves_right();
        LUnits xLeft = pInstrEngrv->get_staves_left();
        LUnits yTop = pInstrEngrv->get_top_line_of_staff(iStaff);
        pEngrv->set_start_staffobj(pRO, pSO, pStaffObjShape, iInstr, iStaff,
                                   iSystem, iCol, xLeft, xRight, yTop);
        m_shapesStorage.save_engraver(pEngrv, pRO);
    }
}

//---------------------------------------------------------------------------------------
void ShapesCreator::continue_engraving_relobj(ImoRelObj* pRO,
                                              ImoStaffObj* pSO,
                                              GmoShape* pStaffObjShape, int iInstr,
                                              int iStaff, int iSystem, int iCol,
                                              int UNUSED(iLine),
                                              ImoInstrument* UNUSED(pInstr))
{
    InstrumentEngraver* pInstrEngrv = m_pPartsEngraver->get_engraver_for(iInstr);
    LUnits xRight = pInstrEngrv->get_staves_right();
    LUnits xLeft = pInstrEngrv->get_staves_left();
    LUnits yTop = pInstrEngrv->get_top_line_of_staff(iStaff);

    RelObjEngraver* pEngrv
        = static_cast<RelObjEngraver*>(m_shapesStorage.get_engraver(pRO));
    pEngrv->set_middle_staffobj(pRO, pSO, pStaffObjShape, iInstr, iStaff, iSystem, iCol,
                                xLeft, xRight, yTop);
}

//---------------------------------------------------------------------------------------
void ShapesCreator::finish_engraving_relobj(ImoRelObj* pRO,
                                            ImoStaffObj* pSO,
                                            GmoShape* pStaffObjShape,
                                            int iInstr, int iStaff, int iSystem,
                                            int iCol, int UNUSED(iLine),
                                            LUnits prologWidth,
                                            ImoInstrument* UNUSED(pInstr))
{
    InstrumentEngraver* pInstrEngrv = m_pPartsEngraver->get_engraver_for(iInstr);
    LUnits xRight = pInstrEngrv->get_staves_right();
    LUnits xLeft = pInstrEngrv->get_staves_left();
    LUnits yTop = pInstrEngrv->get_top_line_of_staff(iStaff);

    RelObjEngraver* pEngrv
        = static_cast<RelObjEngraver*>(m_shapesStorage.get_engraver(pRO));
    pEngrv->set_end_staffobj(pRO, pSO, pStaffObjShape, iInstr, iStaff, iSystem, iCol,
                             xLeft, xRight, yTop);
    pEngrv->set_prolog_width( prologWidth );

    pEngrv->create_shapes(pRO->get_color());
}

//---------------------------------------------------------------------------------------
void ShapesCreator::start_engraving_auxrelobj(ImoAuxRelObj* pARO, ImoStaffObj* pSO,
                                              const string& tag, GmoShape* pStaffObjShape,
                                              int iInstr, int iStaff, int iSystem,
                                              int iCol, int UNUSED(iLine),
                                              ImoInstrument* UNUSED(pInstr))
{
    //factory method to create the engraver for AuxRelObjs

    AuxRelObjEngraver* pEngrv = NULL;
    switch (pARO->get_obj_type())
    {
        case k_imo_lyric:
        {
            InstrumentEngraver* pInstrEngrv = m_pPartsEngraver->get_engraver_for(iInstr);
            pEngrv = LOMSE_NEW LyricEngraver(m_libraryScope, m_pScoreMeter, pInstrEngrv);
            break;
        }

        default:
            return;
    }

    if (pEngrv)
    {
        InstrumentEngraver* pInstrEngrv = m_pPartsEngraver->get_engraver_for(iInstr);
        LUnits xRight = pInstrEngrv->get_staves_right();
        LUnits xLeft = pInstrEngrv->get_staves_left();
        LUnits yTop = pInstrEngrv->get_top_line_of_staff(iStaff);
        pEngrv->set_start_staffobj(pARO, pSO, pStaffObjShape, iInstr, iStaff,
                                   iSystem, iCol, xLeft, xRight, yTop);
        m_shapesStorage.save_engraver(pEngrv, tag);
    }
}

//---------------------------------------------------------------------------------------
void ShapesCreator::continue_engraving_auxrelobj(ImoAuxRelObj* pARO, ImoStaffObj* pSO,
                                              const string& tag, GmoShape* pStaffObjShape,
                                              int iInstr, int iStaff, int iSystem,
                                              int iCol, int UNUSED(iLine),
                                              ImoInstrument* UNUSED(pInstr))
{
    InstrumentEngraver* pInstrEngrv = m_pPartsEngraver->get_engraver_for(iInstr);
    LUnits xRight = pInstrEngrv->get_staves_right();
    LUnits xLeft = pInstrEngrv->get_staves_left();
    LUnits yTop = pInstrEngrv->get_top_line_of_staff(iStaff);

    AuxRelObjEngraver* pEngrv
        = static_cast<AuxRelObjEngraver*>(m_shapesStorage.get_engraver(tag));
    pEngrv->set_middle_staffobj(pARO, pSO, pStaffObjShape, iInstr, iStaff, iSystem, iCol,
                                xLeft, xRight, yTop);
}

//---------------------------------------------------------------------------------------
void ShapesCreator::finish_engraving_auxrelobj(ImoAuxRelObj* pARO, ImoStaffObj* pSO,
                                               const string& tag, GmoShape* pStaffObjShape,
                                               int iInstr, int iStaff, int iSystem,
                                               int iCol, int UNUSED(iLine),
                                               LUnits prologWidth,
                                               ImoInstrument* UNUSED(pInstr))
{
    InstrumentEngraver* pInstrEngrv = m_pPartsEngraver->get_engraver_for(iInstr);
    LUnits xRight = pInstrEngrv->get_staves_right();
    LUnits xLeft = pInstrEngrv->get_staves_left();
    LUnits yTop = pInstrEngrv->get_top_line_of_staff(iStaff);

    AuxRelObjEngraver* pEngrv
        = static_cast<AuxRelObjEngraver*>(m_shapesStorage.get_engraver(tag));
    pEngrv->set_end_staffobj(pARO, pSO, pStaffObjShape, iInstr, iStaff, iSystem, iCol,
                             xLeft, xRight, yTop);
    pEngrv->set_prolog_width( prologWidth );

    pEngrv->create_shapes(pARO->get_color());
}

////---------------------------------------------------------------------------------------
//void ShapesCreator::create_shapes_for_lyrics(ImoStaffObj* pSO, GmoShapeNote* pNoteShape,
//                                             int iInstr, int iStaff)
//{
//    ImoAttachments* pAuxObjs = pSO->get_attachments();
//    if (!pAuxObjs)
//        return;
//
//    ImoNote* pNote = static_cast<ImoNote*>(pSO);
//    int size = pAuxObjs->get_num_items();
//    for (int i=0; i < size; ++i)
//    {
//        ImoAuxObj* pAO = static_cast<ImoAuxObj*>( pAuxObjs->get_item(i) );
//        if (pAO->is_lyric())
//        {
//            ImoLyric* pLyric = static_cast<ImoLyric*>(pAO);
//
//            //build hash code from instrument, number & voice.
//            stringstream tag;
//            tag << iInstr << "-" << pLyric->get_number() << "-" << pNote->get_voice();
//            string id = tag.str();
//
//            //find engraver or create one
//            LyricEngraver* pEngrv;
//            if (pLyric->is_start_of_lyrics_line())
//            {
//                pEngrv = LOMSE_NEW LyricEngraver(m_libraryScope, m_pScoreMeter);
//                m_lyricEngravers[id] = pEngrv;
//            }
//            else
//            {
//                //get engraver for this lyrics line
//                map<string, LyricEngraver*>::iterator it = m_lyricEngravers.find(id);
//                if (it == m_lyricEngravers.end())
//                {
//                    LOMSE_LOG_ERROR("No engraver!!!!!");
//                    return;
//                }
//                else
//                    pEngrv = it->second;
//            }
//
//            GmoShape* pAuxShape = pEngrv->create_Lyric_shape(pLyric, pNoteShape);
////            add_aux_shape_to_model(pAuxShape, GmoShape::k_layer_aux_objs, iSystem,
////                                      iCol, iInstr);
//         }
//
//    }
//}


//=======================================================================================
// LinesBreakerSimple implementation
//=======================================================================================
LinesBreakerSimple::LinesBreakerSimple(ScoreLayouter* pScoreLyt,
                                       SpacingAlgorithm* pSpAlgorithm,
                                       std::vector<int>& breaks)
    : LinesBreaker(pScoreLyt, pSpAlgorithm, breaks)
{
}

//---------------------------------------------------------------------------------------
void LinesBreakerSimple::decide_line_breaks()
{
    //simple algorithm: just fill system with columns while space available

    int numCols = m_pScoreLyt->get_num_columns();
    int iSystem = 0;

    //start first system
    m_breaks.push_back(0);
    LUnits space = m_pScoreLyt->get_target_size_for_system(0)
                   - m_pScoreLyt->get_column_width(0);        //+gross

    for (int iCol=1; iCol < numCols; ++iCol)
    {
        LUnits colSize = m_pScoreLyt->get_column_width(iCol);     //+gross
        if (space >= colSize && !m_pScoreLyt->column_has_system_break(iCol))
            space -= colSize;
        else
        {
            //start new system
            iSystem++;
            m_breaks.push_back(iCol);
            space = m_pScoreLyt->get_target_size_for_system(iSystem) - colSize;
        }
    }
}



//=======================================================================================
// LinesBreakerOptimal implementation
//=======================================================================================

LinesBreakerOptimal::LinesBreakerOptimal(ScoreLayouter* pScoreLyt,
                                         SpacingAlgorithm* pSpAlgorithm,
                                         std::vector<int>& breaks)
    : LinesBreaker(pScoreLyt, pSpAlgorithm, breaks)
    , m_numCols(0)
    , m_fJustifyLastLine(false)
{
}

//---------------------------------------------------------------------------------------
void LinesBreakerOptimal::decide_line_breaks()
{
    //algorithm, very closely related to Knuths' algorithm for breaking lines in
    //word processor systems, as described in [GUIDO]

    initialize_entries_table();
    compute_optimal_break_sequence();
    retrieve_breaks_sequence();
}

//---------------------------------------------------------------------------------------
void LinesBreakerOptimal::initialize_entries_table()
{
    m_numCols = m_pScoreLyt->get_num_columns();

    m_entries.reserve(m_numCols+1);
    m_entries.assign(m_numCols+1, Entry());
    m_entries[0].penalty = 0.0f;
    m_entries[0].predecessor = 0;
    m_entries[0].system = 0;
    for (int i=1; i <= m_numCols; ++i)
    {
        m_entries[i].penalty = LOMSE_INFINITE_PENALTY;
        m_entries[i].predecessor = -1;
        m_entries[i].system = 0;
    }
}

//---------------------------------------------------------------------------------------
void LinesBreakerOptimal::compute_optimal_break_sequence()
{
    for (int i=0; i < m_numCols; ++i)
    {

//        dbgLogger << "Breaks i loop. "
//                  << "Entry " << i << ": prev = " << m_entries[i].predecessor
//                  << ", penalty = " << m_entries[i].penalty
//                  << ", system = " << m_entries[i].system << endl;

        if (m_entries[i].penalty < LOMSE_INFINITE_PENALTY)
        {
            int iSystem = m_entries[i].system;
            float prevPenalty = m_entries[i].penalty;
            for (int j=i+1; j <= m_numCols; ++j)
            {
//                dbgLogger << "Breaks j loop. "
//                          << "Entry " << j << ": prev = " << m_entries[j].predecessor
//                          << ", penalty = " << m_entries[j].penalty
//                          << ", system = " << m_entries[j].system << endl;

                //try system formed by columns {ci,...,cj-1}

                bool fSystemBreak = m_pScoreLyt->column_has_system_break(j-1);
                float newPenalty;
                if (fSystemBreak)
                {
                    newPenalty = 0.0f;
                    prevPenalty = 0.0f;
                }
                else
                    newPenalty = m_pSpAlgorithm->determine_penalty_for_line(iSystem, i, j-1);

//                dbgLogger << "Penalty for (" << i << ", " << j << ")= "
//                          << (prevPenalty + newPenalty) << ". Current= "
//                          << m_entries[j].penalty << endl;

                if (fSystemBreak || m_pSpAlgorithm->is_better_option(prevPenalty, newPenalty,
                                                            m_entries[j].penalty, i, j-1))
                {
//                    dbgLogger << (prevPenalty + newPenalty) << " is better option than "
//                              << m_entries[j].penalty << " for entry " << j << endl;

                    m_entries[j].penalty = newPenalty + prevPenalty;
                    m_entries[j].predecessor = i;
                    m_entries[j].system = iSystem + 1;
                }

                if (fSystemBreak)
                    break;

                //optimization: if no space for column j do not try column j+1
                if (newPenalty >= LOMSE_INFINITE_PENALTY)
                    break;
            }
        }
    }
}

//---------------------------------------------------------------------------------------
void LinesBreakerOptimal::retrieve_breaks_sequence()
{
//    dbgLogger << "Breaks computed. Entries: ************************************" << endl;
//    dump_entries(dbgLogger);

    int i = m_numCols;
    while (i > 0 && m_entries[i].predecessor <= 0)
        --i;

    if (i == 0)
    {
        //no breaks. Just one single system
        m_breaks.push_back(0);      //AWARE: breaks size is the number of systems because
                                    //last break is implicit: last column

//        dbgLogger << "Breaks table ************************************" << endl;
//        dbgLogger << "No breaks, just one single system. breaks size= " << m_breaks.size() << endl;
        return;
    }

    int numBreaks = m_entries[i].system;
    m_breaks.reserve(numBreaks);
    m_breaks.assign(numBreaks, 0);

    while (m_entries[i].predecessor > 0)
    {
        i = m_entries[i].predecessor;
        m_breaks[--numBreaks] = i;
    }
//    dbgLogger << "Breaks table ************************************" << endl;
//    vector<int>::iterator it=m_breaks.begin();
//    for (++it; it != m_breaks.end(); ++it)
//        dbgLogger << "break at column " << *it << endl;
}

//---------------------------------------------------------------------------------------
void LinesBreakerOptimal::dump_entries(ostream& outStream)
{
    int numEntries = int(m_entries.size());
    for (int i=0; i < numEntries; ++i)
    {
        outStream << "Entry " << i << ": prev = " << m_entries[i].predecessor
                  << ", penalty = " << m_entries[i].penalty
                  << ", system = " << m_entries[i].system << endl;
    }
}


}  //namespace lomse
