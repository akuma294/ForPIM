/*
 * Copyright (c) 2011 ARM Limited
 * All rights reserved.
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2002-2005 The Regents of The University of Michigan
 * Copyright (c) 2009 The University of Edinburgh
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Gabe Black
 *          Timothy M. Jones
 */

#ifndef __CPU_TRANSLATION_HH__
#define __CPU_TRANSLATION_HH__

#include "arch/generic/tlb.hh"
#include "sim/faults.hh"

/**
 * This class captures the state of an address translation.  A translation
 * can be split in two if the ISA supports it and the memory access crosses
 * a page boundary.  In this case, this class is shared by two data
 * translations (below).  Otherwise it is used by a single data translation
 * class.  When each part of the translation is finished, the finish
 * function is called which will indicate whether the whole translation is
 * completed or not.  There are also functions for accessing parts of the
 * translation state which deal with the possible split correctly.
 */
class WholeTranslationState
{
  protected:
    int outstanding;
    Fault faults[3];

  public:
    bool delay;
    bool isSplit;
    bool isRowOp;
    RequestPtr mainReq;
    RequestPtr sreqLow;
    RequestPtr sreqHigh;
    RequestPtr sreqDest;
    RequestPtr sreqSrc1;
    RequestPtr sreqSrc2;
    uint8_t *data;
    uint64_t *res;
    BaseTLB::Mode mode;

    /**
     * Single translation state.  We set the number of outstanding
     * translations to one and indicate that it is not split.
     */
    WholeTranslationState(RequestPtr _req, uint8_t *_data, uint64_t *_res,
                          BaseTLB::Mode _mode)
        : outstanding(1), delay(false), isSplit(false), isRowOp(false),
          mainReq(_req), sreqLow(NULL), sreqHigh(NULL), sreqDest(NULL),
          sreqSrc1(NULL), sreqSrc2(NULL), data(_data), res(_res), mode(_mode)
    {
        faults[0] = faults[1] = faults[2] = NoFault;
        assert(mode == BaseTLB::Read || mode == BaseTLB::Write);
    }

    /**
     * Split translation state.  We copy all state into this class, set the
     * number of outstanding translations to two and then mark this as a
     * split translation.
     */
    WholeTranslationState(RequestPtr _req, RequestPtr _sreqLow,
                          RequestPtr _sreqHigh, uint8_t *_data, uint64_t *_res,
                          BaseTLB::Mode _mode)
        : outstanding(2), delay(false), isSplit(true), isRowOp(false),
          mainReq(_req), sreqLow(_sreqLow), sreqHigh(_sreqHigh), sreqDest(NULL),
          sreqSrc1(NULL), sreqSrc2(NULL), data(_data), res(_res), mode(_mode)
    {
        faults[0] = faults[1] = faults[2] = NoFault;
        assert(mode == BaseTLB::Read || mode == BaseTLB::Write);
    }

    /**
     * Triple (or double) translation state for row op.
     */
    WholeTranslationState(RequestPtr _req, RequestPtr _sreqDest,
                          RequestPtr _sreqSrc1, RequestPtr _sreqSrc2,
                          uint8_t *_data, uint64_t *_res,
                          BaseTLB::Mode _mode)
        : outstanding(_sreqSrc1 == NULL? 1 : (_sreqSrc2 == NULL? 2 : 3)), delay(false), isSplit(false),
          isRowOp(true), mainReq(_req), sreqLow(NULL), sreqHigh(NULL),
          sreqDest(_sreqDest), sreqSrc1(_sreqSrc1), sreqSrc2(_sreqSrc2),
          data(_data), res(_res), mode(_mode)
    {
        faults[0] = faults[1] = faults[2] = NoFault;
        assert(mode == BaseTLB::Write);
    }

    /**
     * Finish part of a translation.  If there is only one request then this
     * translation is completed.  If the request has been split in two then
     * the outstanding count determines whether the translation is complete.
     * In this case, flags from the split request are copied to the main
     * request to make it easier to access them later on.
     */
    bool
    finish(const Fault &fault, int index)
    {
        assert(outstanding);
        faults[index] = fault;
        outstanding--;
        if (isSplit && outstanding == 0) {

            // For ease later, we copy some state to the main request.
            if (faults[0] == NoFault) {
                mainReq->setPaddr(sreqLow->getPaddr());
            }
            mainReq->setFlags(sreqLow->getFlags());
            mainReq->setFlags(sreqHigh->getFlags());
        }
        if (isRowOp && outstanding == 0) {
            Request::RowOpPayload* addrs = (Request::RowOpPayload*) data;
            if (faults[0] == NoFault) {
                addrs->dest = sreqDest->getPaddr();
            }
            mainReq->setFlags(sreqDest->getFlags());
			if (sreqSrc1 != NULL) {
            	if (faults[1] == NoFault) {
                	addrs->src1 = sreqSrc1->getPaddr();
            	}
				mainReq->setFlags(sreqSrc1->getFlags());
			}
            if (sreqSrc2 != NULL) {
                if (faults[2] == NoFault) {
                    addrs->src2 = sreqSrc2->getPaddr();
                }
                mainReq->setFlags(sreqSrc2->getFlags());
            }
            mainReq->setPaddr(0);
        }
        return outstanding == 0;
    }

    /**
     * Determine whether this translation produced a fault.  Both parts of the
     * translation must be checked if this is a split translation.
     */
    Fault
    getFault() const
    {
        if (!isSplit)
            return faults[0];
        else if (faults[0] != NoFault)
            return faults[0];
        else if (faults[1] != NoFault)
            return faults[1];
        else if (faults[2] != NoFault)
            return faults[2];
        else
            return NoFault;
    }

    /** Remove all faults from the translation. */
    void
    setNoFault()
    {
        faults[0] = faults[1] = faults[2] = NoFault;
    }

    /**
     * Check if this request is strictly ordered device access.  We
     * only need to check the main request because the flags will have
     * been copied here on a split translation.
     */
    bool
    isStrictlyOrdered() const
    {
        return mainReq->isStrictlyOrdered();
    }

    /**
     * Check if this request is a prefetch.  We only need to check the main
     * request because the flags will have been copied here on a split
     * translation.
     */
    bool
    isPrefetch() const
    {
        return mainReq->isPrefetch();
    }

    /** Get the physical address of this request. */
    Addr
    getPaddr() const
    {
        return mainReq->getPaddr();
    }

    /**
     * Get the flags associated with this request.  We only need to access
     * the main request because the flags will have been copied here on a
     * split translation.
     */
    unsigned
    getFlags()
    {
        return mainReq->getFlags();
    }

    /** Delete all requests that make up this translation. */
    void
    deleteReqs()
    {
        delete mainReq;
        if (isSplit) {
            delete sreqLow;
            delete sreqHigh;
        }
        if (isRowOp) {
            delete sreqDest;
            delete sreqSrc1;
            delete sreqSrc2;
        }
    }
};


/**
 * This class represents part of a data address translation.  All state for
 * the translation is held in WholeTranslationState (above).  Therefore this
 * class does not need to know whether the translation is split or not.  The
 * index variable determines this but is simply passed on to the state class.
 * When this part of the translation is completed, finish is called.  If the
 * translation state class indicate that the whole translation is complete
 * then the execution context is informed.
 */
template <class ExecContextPtr>
class DataTranslation : public BaseTLB::Translation
{
  protected:
    ExecContextPtr xc;
    WholeTranslationState *state;
    int index;

  public:
    DataTranslation(ExecContextPtr _xc, WholeTranslationState* _state)
        : xc(_xc), state(_state), index(0)
    {
    }

    DataTranslation(ExecContextPtr _xc, WholeTranslationState* _state,
                    int _index)
        : xc(_xc), state(_state), index(_index)
    {
    }

    /**
     * Signal the translation state that the translation has been delayed due
     * to a hw page table walk.  Split requests are transparently handled.
     */
    void
    markDelayed()
    {
        state->delay = true;
    }

    /**
     * Finish this part of the translation and indicate that the whole
     * translation is complete if the state says so.
     */
    void
    finish(const Fault &fault, RequestPtr req, ThreadContext *tc,
           BaseTLB::Mode mode)
    {
        assert(state);
        assert(mode == state->mode);
        if (state->finish(fault, index)) {
            if (state->getFault() == NoFault) {
                // Don't access the request if faulted (due to squash)
                req->setTranslateLatency();
            }
            xc->finishTranslation(state);
        }
        delete this;
    }

    bool
    squashed() const
    {
        return xc->isSquashed();
    }
};

#endif // __CPU_TRANSLATION_HH__
