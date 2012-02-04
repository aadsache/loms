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

#include "lomse_beam_engraver.h"

#include "lomse_glyphs.h"
#include "lomse_shape_beam.h"
#include "lomse_score_meter.h"
#include "lomse_engraving_options.h"
#include "lomse_im_note.h"
#include "lomse_gm_basic.h"
#include "lomse_shape_note.h"

#include <cmath>        // fabs


namespace lomse
{

//---------------------------------------------------------------------------------------
// BeamEngraver implementation
//---------------------------------------------------------------------------------------
BeamEngraver::BeamEngraver(LibraryScope& libraryScope, ScoreMeter* pScoreMeter)
    : RelAuxObjEngraver(libraryScope, pScoreMeter)
    , m_pBeamShape(NULL)
{
}

//---------------------------------------------------------------------------------------
BeamEngraver::~BeamEngraver()
{
    m_noteRests.clear();
}

//---------------------------------------------------------------------------------------
void BeamEngraver::set_start_staffobj(ImoAuxObj* pAO, ImoStaffObj* pSO,
                                        GmoShape* pStaffObjShape, int iInstr, int iStaff,
                                        int iSystem, int iCol, UPoint pos)
{
    m_iInstr = iInstr;
    m_iStaff = iStaff;
    m_pBeam = dynamic_cast<ImoBeam*>(pAO);

    ImoNoteRest* pNR = dynamic_cast<ImoNoteRest*>(pSO);
    m_noteRests.push_back( make_pair(pNR, pStaffObjShape) );

    m_shapesInfo[0].iCol = iCol;
    m_shapesInfo[0].iInstr = iInstr;
    m_shapesInfo[0].iSystem = iSystem;
}

//---------------------------------------------------------------------------------------
void BeamEngraver::set_middle_staffobj(ImoAuxObj* pAO, ImoStaffObj* pSO,
                                         GmoShape* pStaffObjShape, int iInstr,
                                         int iStaff, int iSystem, int iCol)
{
    ImoNoteRest* pNR = dynamic_cast<ImoNoteRest*>(pSO);
    m_noteRests.push_back( make_pair(pNR, pStaffObjShape) );
}

//---------------------------------------------------------------------------------------
void BeamEngraver::set_end_staffobj(ImoAuxObj* pAO, ImoStaffObj* pSO,
                                      GmoShape* pStaffObjShape, int iInstr, int iStaff,
                                      int iSystem, int iCol)
{
    ImoNoteRest* pNR = dynamic_cast<ImoNoteRest*>(pSO);
    m_noteRests.push_back( make_pair(pNR, pStaffObjShape) );
}

//---------------------------------------------------------------------------------------
int BeamEngraver::create_shapes()
{
    decide_on_stems_direction();
    decide_beam_position();
    change_stems_direction();
    adjust_stems_lengths();
    reposition_rests();
    compute_beam_segments();
    create_shape();
    add_shape_to_noterests();
    return 1;
}

//---------------------------------------------------------------------------------------
void BeamEngraver::create_shape()
{
    m_pBeamShape = LOMSE_NEW GmoShapeBeam(m_pBeam, m_uBeamThickness);
    m_pBeamShape->set_layout_data(m_segments, m_origin, m_size,
                                  m_outerLeftPoint, m_outerRightPoint);

    m_pShape = m_pBeamShape;
    m_shapesInfo[0].pShape = m_pBeamShape;
}

//---------------------------------------------------------------------------------------
void BeamEngraver::add_shape_to_noterests()
{
    std::list< pair<ImoNoteRest*, GmoShape*> >::iterator it;
    for(it=m_noteRests.begin(); it != m_noteRests.end(); ++it)
        (it->second)->add_related_shape(m_pBeamShape);
}

//---------------------------------------------------------------------------------------
void BeamEngraver::reposition_rests()
{
    //compute the average position of all noteheads.
    int numNotes = 0;
    int posForRests = 0;
    std::list< pair<ImoNoteRest*, GmoShape*> >::iterator it;
    for(it=m_noteRests.begin(); it != m_noteRests.end(); ++it)
	{
        if ((it->first)->is_note())
        {
            GmoShapeNote* pShapeNote = dynamic_cast<GmoShapeNote*>(it->second);
            numNotes++;
            posForRests += pShapeNote->get_pos_on_staff();
        }
    }
    posForRests /= numNotes;

    //to convert to tenths it is necesary to multiply by 10/2 = 5
    Tenths meanPos = Tenths(posForRests) * 5.0f;

    // As rests are normally positioned on 3rd space (35 tenths), the shift to apply is
    Tenths tShift = 35.0f - meanPos;
    USize shift(0.0f, m_pMeter->tenths_to_logical(tShift, m_iInstr, m_iStaff));

    //shift rests
    for(it=m_noteRests.begin(); it != m_noteRests.end(); ++it)
	{
        if ((it->first)->is_rest())
        {
            GmoShapeRest* pShapeRest = dynamic_cast<GmoShapeRest*>(it->second);
            pShapeRest->shift_origin(shift);
        }
    }
}

//---------------------------------------------------------------------------------------
void BeamEngraver::decide_on_stems_direction()
{
    //look for the stem direction of most notes. If one note has its stem direction
    //forced (by a tie, probably) forces the beam stems in this direction

    m_fStemForced = false;     //assume no stem forced
    m_fStemMixed = false;      //assume all stems in the same direction
    m_fStemsDown = false;      //set stems up by default
    m_numStemsDown = 0;
    m_numNotes = 0;
    m_averagePosOnStaff = 0;

    std::list< pair<ImoNoteRest*, GmoShape*> >::iterator it;
    for(it=m_noteRests.begin(); it != m_noteRests.end(); ++it)
	{
        if ((it->first)->is_note())      //ignore rests
        {
		    ImoNote* pNote = dynamic_cast<ImoNote*>(it->first);
            m_numNotes++;

            if (pNote->get_stem_direction() != k_stem_default)
            {
                m_fStemForced = true;
                m_fStemsDown = pNote->is_stem_down();
                //stem forced by last forced stem
            }
            else
            {
                GmoShapeNote* pNoteShape = dynamic_cast<GmoShapeNote*>(it->second);
                m_averagePosOnStaff += pNoteShape->get_pos_on_staff();
                GmoShapeStem* pStemShape = pNoteShape->get_stem_shape();
                if (pStemShape && pStemShape->is_stem_down())
                    m_numStemsDown++;
            }
        }
    }

    if (!m_fStemForced)
    {
        m_fStemsDown = m_averagePosOnStaff / m_numNotes > 6;
        m_fStemMixed = false;   //TODO: For now no automatic mixed beams
    }
    else
    {
        m_fStemMixed = (m_numStemsDown !=0 && m_numStemsDown != m_numNotes);
    }
}

//---------------------------------------------------------------------------------------
void BeamEngraver::decide_beam_position()
{
    m_fBeamAbove = !m_fStemsDown;
}

//---------------------------------------------------------------------------------------
void BeamEngraver::change_stems_direction()
{
    //TODO
    ////correct beam position (and reverse stems direction) if first note of beamed
    ////group is tied to a previous note and the stems' directions are not forced
    //if (!m_fStemForced && m_noteRests.front()->is_note())
    //{
    //    ImoNote* pFirst = (ImoNote*)m_noteRests.front();
    //    if (pFirst->IsTiedToPrev())
    //        m_fStemsDown = pFirst->GetTiedNotePrev()->is_stem_down();
    //}

    //the beam line position is going to be established by the first and last
    //notes stems. Therefore, if stems directions are not fixed, it is necessary to
    //change stem directions of notes
    if (!m_fStemForced)
    {
        std::list< pair<ImoNoteRest*, GmoShape*> >::iterator it;
        for(it=m_noteRests.begin(); it != m_noteRests.end(); ++it)
	    {
            if (it->first->is_note())
            {
                GmoShapeNote* pShape = dynamic_cast<GmoShapeNote*>(it->second);
                pShape->set_stem_down(m_fStemsDown);
            }
        }
    }
}

//---------------------------------------------------------------------------------------
void BeamEngraver::compute_beam_segments()
{
	//AWARE: stems length are already trimmed

    m_uBeamThickness = m_pMeter->tenths_to_logical(LOMSE_BEAM_THICKNESS, m_iInstr, m_iStaff);
    LUnits uBeamSpacing = m_pMeter->tenths_to_logical(LOMSE_BEAM_SPACING, m_iInstr, m_iStaff)
                          + m_uBeamThickness;
    LUnits uBeamHookLength = m_pMeter->tenths_to_logical(LOMSE_BEAM_HOOK_LENGTH, m_iInstr, m_iStaff);

    LUnits uxPrev=0, uyPrev=0, uxCur=0, uyCur=0;    // points for previous and current note
    LUnits uyShift = 0;     // shift, to separate a beam line from the previous one

    bool fForwardPending = false;           //finish a Forward hook in prev note

    //For re-computing bounds
    m_origin.x = 100000000.0f;       //any too big value
    m_origin.y = 100000000.0f;      //any too big value
    m_size.width = 0.0f;
    m_size.height = 0.0f;

    LUnits uHalfBeam = m_uBeamThickness / 2.0f;

    //compute beam segments
    for (int iLevel=0; iLevel < 6; iLevel++)
	{
        LUnits uxStart=0, uxEnd=0, uyStart=0, uyEnd=0; // start and end points for a segment
        bool fStartPointComputed = false;
        bool fEndPointComputed = false;


        std::list< pair<ImoStaffObj*, ImoRelDataObj*> >& beamData
            = m_pBeam->get_related_objects();
        std::list< pair<ImoStaffObj*, ImoRelDataObj*> >::iterator itLD = beamData.begin();
        std::list< pair<ImoNoteRest*, GmoShape*> >::iterator itNR;
        for(itNR = m_noteRests.begin(); itNR != m_noteRests.end(); ++itNR, ++itLD)
        {
            ImoBeamData* pBeamData = dynamic_cast<ImoBeamData*>( (*itLD).second );
            GmoShape* pNR = (*itNR).second;
            if (pNR->is_shape_note())
            {
                GmoShapeNote* pShapeNote = dynamic_cast<GmoShapeNote*>(pNR);

                uxCur = pShapeNote->get_stem_left();
                uyCur = pShapeNote->get_stem_y_flag() + uyShift;

                //Let's check if we have to finish a forward hook in prev. note
                if (fForwardPending)
                {
                    uxEnd = uxPrev + uBeamHookLength;
                    uyEnd = uyPrev + uBeamHookLength*(uyCur-uyPrev)/(uxCur-uxPrev);
                    add_segment(uxStart, uyStart, uxEnd, uyEnd);
                    update_bounds(uxStart, uyStart, uxEnd, uyEnd);
                    fForwardPending = false;
                }

                // now we can deal with current note
                switch ( pBeamData->get_beam_type(iLevel) ) //pShapeNote->get_beam_type(iLevel) )
                {
                    case ImoBeam::k_begin:
                        //start of segment. Compute initial point
                        fStartPointComputed = true;
                        uxStart = uxCur;
                        uyStart = uyCur;
                        break;

                    case ImoBeam::k_end:
                        // end of segment. Compute end point
                        fEndPointComputed = true;
                        uxEnd = uxCur;
                        uyEnd = uyCur;
                        break;

                    case ImoBeam::k_forward:
                        // start of segment. A forward hook is pending.
                        // compute its initial point
                        fForwardPending = true;
                        uxStart = uxCur;
                        uyStart = uyCur;
                        break;

                    case ImoBeam::k_backward:
                        // end of segment. compute start and end points
                        uxEnd = uxCur;
                        uyEnd = uyCur;
                        uxStart = uxCur - uBeamHookLength;
                        uyStart = uyPrev + (uxCur-uxPrev-uBeamHookLength)*(uyCur-uyPrev)/(uxCur-uxPrev);
                        fStartPointComputed = true;      //mark 'segment ready to be drawn'
                        fEndPointComputed = true;
                    break;

                    case ImoBeam::k_continue:
                    case ImoBeam::k_none:
                        // nothing to do.
                        break;
                }

                // if we have data to draw a segment, draw it
                if (fStartPointComputed && fEndPointComputed)
			    {
				    uxEnd += pShapeNote->get_stem_width();
                    add_segment(uxStart, uyStart, uxEnd, uyEnd);
                    update_bounds(uxStart, uyStart, uxEnd, uyEnd);
                    fStartPointComputed = false;
                    fEndPointComputed = false;
                    if (iLevel == 0)
                    {
                        if (m_fBeamAbove)
                        {
                            m_outerLeftPoint = UPoint(uxStart, uyStart - uHalfBeam);
                            m_outerRightPoint = UPoint(uxEnd, uyEnd - uHalfBeam);
                        }
                        else
                        {
                            m_outerLeftPoint = UPoint(uxStart, uyStart + uHalfBeam);
                            m_outerRightPoint = UPoint(uxEnd, uyEnd + uHalfBeam);
                        }
                    }
                }

                // save position of current note
                uxPrev = uxCur;
                uyPrev = uyCur;
            }
        }

        // displace y coordinate for next beamline
        uyShift += (m_fBeamAbove ? uBeamSpacing : - uBeamSpacing);
    }

    //take beam thickness into accoun for boundaries
    m_origin.y -= uHalfBeam;
    m_size.height += m_uBeamThickness;
}

//---------------------------------------------------------------------------------------
void BeamEngraver::add_segment(LUnits uxStart, LUnits uyStart, LUnits uxEnd, LUnits uyEnd)
{
    m_segments.push_back(uxStart);
    m_segments.push_back(uyStart);
    m_segments.push_back(uxEnd);
    m_segments.push_back(uyEnd);
}

//---------------------------------------------------------------------------------------
void BeamEngraver::update_bounds(LUnits uxStart, LUnits uyStart,
                                 LUnits uxEnd, LUnits uyEnd)
{
    LUnits left = min(uxStart, uxEnd);
    LUnits top = min(uyStart, uyEnd);
    LUnits bottom = max(uyStart, uyEnd);
    LUnits right = max(uxStart, uxEnd);

    m_origin.x = min(m_origin.x, left);
    m_origin.y = min(m_origin.y, top);

    m_size.width = max(m_size.width, right - m_origin.x);
    m_size.height = max(m_size.height, bottom - m_origin.y);
}

//---------------------------------------------------------------------------------------
void BeamEngraver::adjust_stems_lengths()
{
	// In this method the length of note stems in a beamed group is adjusted.

    // At this point all stems have the standard size and the stem start and end points
    // are computed (start point = nearest to notehead). In the following loop we
	// retrieve the start and end 'y' coordinates for each stem,
	// and we store them in the auxiliary arrays yNote and yFlag, respectively.

	int nNumNotes = int(m_noteRests.size());
    std::vector<LUnits> yNote(nNumNotes);
    std::vector<LUnits> yFlag(nNumNotes);
    std::vector<GmoShapeNote*> note(nNumNotes);

    LUnits x1, xn;		    // x position of first and last stems, respectively

    int i = 0;              // index to current element
    std::list< pair<ImoNoteRest*, GmoShape*> >::iterator it;
    for(it = m_noteRests.begin(); it != m_noteRests.end(); ++it)
    {
        GmoShape* pNR = (*it).second;
        if (pNR->is_shape_note())
        {
            GmoShapeNote* pShapeNote = dynamic_cast<GmoShapeNote*>(pNR);
            note[i] = pShapeNote;
            if (i == 0)
                x1 = pShapeNote->get_stem_left();
            else
                xn = pShapeNote->get_stem_left();

            yNote[i] = pShapeNote->get_stem_y_note();
            yFlag[i] = pShapeNote->get_stem_y_flag();
            i++;
        }
    }
	nNumNotes = i;
	int n = nNumNotes - 1;	// index to last element

	// In following loop we compute each stem and update its final position.
    // GmoShapeBeam line position is established by the first and last notes' stems. Now
    // let's adjust the intermediate notes' stem lengths to end up in the beam line.
    // This is just a proportional share based on line slope:
    // If (x1,y1) and (xn,yn) are, respectively, the position of first and last notes of
    // the group, the y position of any intermediate note i can be computed as:
    //     Ay = yn-y1
    //     Ax = xn-x1
    //                Ay
    //     yi = y1 + ---- (xi-x1)
    //                Ax
    //
    // The loop is also used to look for the shortest stem

    LUnits Ay = yFlag[n] - yFlag[0];
    LUnits Ax = xn - x1;
    LUnits uMinStem;
    for(int i=0; i < nNumNotes; i++)
    {
        yFlag[i] = yFlag[0] + (Ay * (note[i]->get_stem_left() - x1)) / Ax;

        //compute stem length. For chords we have to substract the stem segment joining
        //all chord notes. This extra length is zero for notes not in chord
        LUnits uStemLength = fabs(yNote[i] - yFlag[i]) - note[i]->get_stem_extra_length();

        //save the shortest stem
        if (i==0)
            uMinStem = uStemLength;
        else
            uMinStem = min(uMinStem, uStemLength);
    }

    // If the pitch of any intermediate note is out of the interval formed by
    // the first note and the last one, then its stem could be too short.
    // For example, consider a beamed group of three notes, the first and the last
    // ones D4 and the middle  one G4; the beam is horizontal, nearly the G4 line;
    // so the midle notehead (G4) would be positioned just on the beam line.
    // To avoid this problem all stems are forced to have a minimum length

    LUnits dyStem = (note[0]->get_stem_height() + note[n]->get_stem_height()) / 2.0f;
    LUnits dyMin = (2.0f * dyStem) / 3.0f;
    bool fAdjust;

    // compare the shortest with this minimun required
    LUnits uyIncr;
    if (uMinStem < dyMin)
    {
        // a stem is smaller than dyMin. Increment all stems.
        uyIncr = dyMin - uMinStem;
        fAdjust = true;
    }
    else if (uMinStem > dyStem)
    {
        // all stems are greater than the standard size. Reduce them.
        //I'm not sure if this case is possible. But it is simple to deal with it
        uyIncr = -(uMinStem - dyStem);
        fAdjust = true;
    }
    else
    {
        fAdjust = false;
    }

    if (fAdjust)
    {
        for (int i = 0; i < nNumNotes; i++)
        {
            if (yNote[i] < yFlag[i])
                yFlag[i] += uyIncr;
             else
                yFlag[i] -= uyIncr;
        }
    }

    // At this point stems' lengths are computed and adjusted.
    // Transfer the computed values to the stem shape
    for(int i=0; i < nNumNotes; i++)
    {
        note[i]->set_stem_length( fabs(yFlag[i] - yNote[i]) );
    }
}


}  //namespace lomse
