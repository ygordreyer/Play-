#pragma once

#include "AlignedAlloc.h"
#include "../COP_SCU.h"
#include "../COP_FPU.h"
#include "DMAC.h"
#include "GIF.h"
#include "SIF.h"
#include "Vpu.h"
#include "IPU.h"
#include "INTC.h"
#include "Timer.h"
#include "Types.h"
#include "MA_VU.h"
#include "MA_EE.h"
#include "COP_VU.h"
#include "PS2OS.h"
#include "../gs/GSHandler.h"

#include "signal/Signal.h"

namespace Ee
{
	class CSubSystem
	{
	public:
		CSubSystem(uint8*, CIopBios&);
		virtual ~CSubSystem();

		void Reset(uint32);
		int ExecuteCpu(int);
		bool IsCpuIdle() const;
		void CountTicks(int);

		void NotifyVBlankStart();
		void NotifyVBlankEnd();

		void SaveState(Framework::CZipArchiveWriter&);
		void LoadState(Framework::CZipArchiveReader&);

		void SetVpu0(std::shared_ptr<CVpu>);
		void SetVpu1(std::shared_ptr<CVpu>);

		uint8* m_ram = nullptr;
		uint8* m_bios = nullptr;
		uint8* m_spr = nullptr;
		uint8* m_fakeIopRam = nullptr;

		uint8* m_vuMem0 = nullptr;
		uint8* m_microMem0 = nullptr;

		uint8* m_vuMem1 = nullptr;
		uint8* m_microMem1 = nullptr;

		CMIPS m_EE;
		CMIPS m_VU0;
		CMIPS m_VU1;

		CGSHandler* m_gs = nullptr;
		CDMAC m_dmac;
		CGIF m_gif;
		CSIF m_sif;
		std::shared_ptr<CVpu> m_vpu0;
		std::shared_ptr<CVpu> m_vpu1;
		CINTC m_intc;
		CIPU m_ipu;
		CTimer m_timer;
		CPS2OS* m_os = nullptr;
		CIopBios& m_iopBios;

		CMemoryMap** GetMemoryMap()
		{
			return &m_EE.m_pMemoryMap;
		}

		void* operator new(size_t allocSize)
		{
			return framework_aligned_alloc(allocSize, 0x10);
		}

		void operator delete(void* ptr)
		{
			return framework_aligned_free(ptr);
		}

	private:
		typedef std::map<uint32, uint32> StatusRegisterCheckerMap;

		void SetupEePageTable();

		uint32 IOPortReadHandler(uint32);
		uint32 IOPortWriteHandler(uint32, uint32);

		uint32 Vu0MicroMemWriteHandler(uint32, uint32);

		uint32 Vu0IoPortReadHandler(uint32);
		uint32 Vu0IoPortWriteHandler(uint32, uint32);
		void Vu0StateChanged(CVpu::VU_STATE);

		uint32 Vu1MicroMemWriteHandler(uint32, uint32);

		uint32 Vu1IoPortReadHandler(uint32);
		uint32 Vu1IoPortWriteHandler(uint32, uint32);

		void CopyVuState(CMIPS&, const CMIPS&);
		uint32 HandleVu1AreaRead(uint32);
		void HandleVu1AreaWrite(uint32, uint32);

		void ExecuteIpu();

		void CheckPendingInterrupts();

		void FlushInstructionCache();

		void LoadBIOS();
		void FillFakeIopRam();

		StatusRegisterCheckerMap m_statusRegisterCheckers;
		bool m_isIdle = false;

		CMA_VU m_MAVU0;
		CMA_VU m_MAVU1;
		CMA_EE m_EEArch;
		CCOP_SCU m_COP_SCU;
		CCOP_FPU m_COP_FPU;
		CCOP_VU m_COP_VU;

		Framework::CSignal<void()>::Connection m_OnRequestInstructionCacheFlushConnection;
		CVpu::VuStateChangedEvent::Connection m_vu0StateChangedConnection;
		CVpu::VuInterruptTriggeredEvent::Connection m_vu1InterruptTriggeredConnection;
	};
};
