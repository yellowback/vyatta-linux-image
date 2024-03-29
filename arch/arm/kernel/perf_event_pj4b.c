/*
 * Marvell Sheeva PJ4B CPU support
 */

#ifdef CONFIG_ARCH_ARMADA_XP 

#define	MRVL_PJ4B_PMU_ENABLE	0x001	/* Enable counters */
#define MRVL_PJ4B_PMN_RESET	0x002	/* Reset event counters */
#define	MRVL_PJ4B_CCNT_RESET	0x004	/* Reset cycles counter */
#define	MRVL_PJ4B_PMU_RESET	(MRVL_PJ4B_CCNT_RESET | MRVL_PJ4B_PMN_RESET)
#define MRVL_PJ4B_PMU_CNT64	0x008	/* Make CCNT count every 64th cycle */




#define PJ4BV7_PMNC_P		(1 << 1) /* Reset all counters */
#define PJ4BV7_PMNC_C		(1 << 2) /* Cycle counter reset */

/*
* Different types of events that can be counted by the Marvell PJ4 Performance Monitor
* 
*/
enum mrvl_pj4b_perf_types  {
	MRVL_PJ4B_SOFTWARE_INCR = 0x00,	/* software increment */
	MRVL_PJ4B_IFU_IFETCH_REFILL = 0x01,	/* instruction fetch that cause a refill at the lowest level of instruction or unified cache */
	MRVL_PJ4B_IF_TLB_REFILL = 0x02,	/* instruction fetch that cause a TLB refill at the lowest level of TLB */
	MRVL_PJ4B_DATA_RW_CACHE_REFILL = 0x03,	/* data read or write operation that causes a refill of at the lowest level of data or unified cache */
	MRVL_PJ4B_DATA_RW_CACHE_ACCESS = 0x04,	/* data read or write operation that causes a cache access at the lowest level of data or unified cache */
	MRVL_PJ4B_DATA_RW_TLB_REFILL = 0x05,	/* data read or write operation that causes a TLB refill at the lowest level of TLB */
	MRVL_PJ4B_DATA_READ_INST_EXEC = 0x06,	/* data read architecturally executed */
	MRVL_PJ4B_DATA_WRIT_INST_EXEC = 0x07,	/* data write architecturally executed */
	MRVL_PJ4B_INSN_EXECUTED = 0x08,	/* instruction architecturally executed */
	MRVL_PJ4B_EXCEPTION_TAKEN = 0x09,	/* exception taken */
	MRVL_PJ4B_EXCEPTION_RETURN = 0x0a,	/* exception return architecturally executed */
	MRVL_PJ4B_INSN_WR_CONTEXTIDR = 0x0b,	/* instruction that writes to the Context ID Register architecturally executed */
	MRVL_PJ4B_SW_CHANGE_PC = 0x0c,	/* software change of PC, except by an exception, architecturally executed */
	MRVL_PJ4B_BR_EXECUTED = 0x0d,	/* immediate branch architecturally executed, taken or not taken */
	MRVL_PJ4B_PROCEDURE_RETURN = 0x0e,	/* procedure return architecturally executed */
	MRVL_PJ4B_UNALIGNED_ACCESS = 0x0f,	/* unaligned access architecturally executed */
	MRVL_PJ4B_BR_INST_MISS_PRED = 0x10,	/* branch mispredicted or not predicted */
	MRVL_PJ4B_CYCLE_COUNT = 0x11,	/* cycle count */
	MRVL_PJ4B_BR_PRED_TAKEN = 0x12,	/* branches or other change in the program flow that could have been predicted by the branch prediction resources of the processor */
	MRVL_PJ4B_DCACHE_READ_HIT = 0x40,	/* counts the number of Data Cache read hits */
	MRVL_PJ4B_DCACHE_READ_MISS = 0x41,	/* connts the number of Data Cache read misses */
	MRVL_PJ4B_DCACHE_WRITE_HIT = 0x42,	/* counts the number of Data Cache write hits */
	MRVL_PJ4B_DCACHE_WRITE_MISS = 0x43,	/* counts the number of Data Cache write misses */
	MRVL_PJ4B_MMU_BUS_REQUEST = 0x44,	/* counts the number of cycles of request to the MMU Bus */
	MRVL_PJ4B_ICACHE_BUS_REQUEST = 0x45,	/* counts the number of cycles the Instruction Cache requests the bus until the data return */
	MRVL_PJ4B_WB_WRITE_LATENCY = 0x46,	/* counts the number of cycles the Write Buffer requests the bus */
	MRVL_PJ4B_HOLD_LDM_STM = 0x47,	/* counts the number of cycles the pipeline is held because of a load/store multiple instruction */
	MRVL_PJ4B_NO_DUAL_CFLAG = 0x48,	/* counts the number of cycles the processor cannot dual issue because of a Carry flag dependency */
	MRVL_PJ4B_NO_DUAL_REGISTER_PLUS = 0x49,	/* counts the number of cycles the processor cannot dual issue because the register file does not have enough read ports and at least one other reason */
	MRVL_PJ4B_LDST_ROB0_ON_HOLD = 0x4a,	/* counts the number of cycles a load or store instruction waits to retire from ROB0 */
	MRVL_PJ4B_LDST_ROB1_ON_HOLD = 0x4b,	/* counts the number of cycles a load or store instruction waits to retire from ROB0=1 */
	MRVL_PJ4B_DATA_WRITE_ACCESS_COUNT = 0x4c, 	/* counts the number of any Data write access */
	MRVL_PJ4B_DATA_READ_ACCESS_COUNT = 0x4d, 	/* counts the number of any Data read access */
	MRVL_PJ4B_A2_STALL = 0x4e, 	/* counts the number of cycles ALU A2 is stalled */
	/*TODO: implement with fabric counters*/
	MRVL_PJ4B_L2C_WRITE_HIT = 0x4f, 	/* counts the number of write accesses to addresses already in the L2C */
	MRVL_PJ4B_L2C_WRITE_MISS = 0x50,	/* counts the number of write accesses to addresses not in the L2C */
	MRVL_PJ4B_L2C_READ_COUNT = 0x51,	/* counts the number of L2C cache-to-bus external read request */
	/*TODO: end*/
	MRVL_PJ4B_ICACHE_READ_MISS = 0x60, 	/* counts the number of Instruction Cache read misses */
	MRVL_PJ4B_ITLB_MISS = 0x61, 	/* counts the number of instruction TLB miss */
	MRVL_PJ4B_SINGLE_ISSUE = 0x62, 	/* counts the number of cycles the processor single issues */
	MRVL_PJ4B_BR_RETIRED = 0x63, 	/* counts the number of times one branch retires */
	MRVL_PJ4B_ROB_FULL = 0x64, 	/* counts the number of cycles the Re-order Buffer (ROB) is full */
	MRVL_PJ4B_MMU_READ_BEAT = 0x65, 	/* counts the number of times the bus returns RDY to the MMU */
	MRVL_PJ4B_WB_WRITE_BEAT = 0x66, 	/* counts the number times the bus returns ready to the Write Buffer */
	MRVL_PJ4B_DUAL_ISSUE = 0x67, 	/* counts the number of cycles the processor dual issues */
	MRVL_PJ4B_NO_DUAL_RAW = 0x68, 	/* counts the number of cycles the processor cannot dual issue because of a Read after Write hazard */
	MRVL_PJ4B_HOLD_IS = 0x69, 	/* counts the number of cycles the issue is held */
	/*TODO: implement with fabric counters*/
	MRVL_PJ4B_L2C_LATENCY = 0x6a, 	/* counts the latency for the most recent L2C read from the external bus Counts cycles */
	/*TODO: end*/
	MRVL_PJ4B_DCACHE_ACCESS = 0x70, 	/* counts the number of times the Data cache is accessed */
	MRVL_PJ4B_DTLB_MISS = 0x71, 	/* counts the number of data TLB misses */
	MRVL_PJ4B_BR_PRED_MISS = 0x72, 	/* counts the number of mispredicted branches */
	MRVL_PJ4B_A1_STALL = 0x74, 	/* counts the number of cycles ALU A1 is stalled */
	MRVL_PJ4B_DCACHE_READ_LATENCY = 0x75, 	/* counts the number of cycles the Data cache requests the bus for a read */
	MRVL_PJ4B_DCACHE_WRITE_LATENCY = 0x76, 	/* counts the number of cycles the Data cache requests the bus for a write */
	MRVL_PJ4B_NO_DUAL_REGISTER_FILE = 0x77, 	/* counts the number of cycles the processor cannot dual issue because the register file doesn't have enough read ports */
	MRVL_PJ4B_BIU_SIMULTANEOUS_ACCESS = 0x78, 	/* BIU Simultaneous Access */
	MRVL_PJ4B_L2C_READ_HIT = 0x79, 	/* counts the number of L2C cache-to-bus external read requests */
	MRVL_PJ4B_L2C_READ_MISS = 0x7a, 	/* counts the number of L2C read accesses that resulted in an external read request */
	MRVL_PJ4B_L2C_EVICTION = 0x7b, 	/* counts the number of evictions (CastOUT) of a line from the L2 cache */
	MRVL_PJ4B_TLB_MISS = 0x80, 	/* counts the number of instruction and data TLB misses */
	MRVL_PJ4B_BR_TAKEN = 0x81, 	/* counts the number of taken branches */
	MRVL_PJ4B_WB_FULL = 0x82, 	/* counts the number of cycles WB is full */
	MRVL_PJ4B_DCACHE_READ_BEAT = 0x83, 	/* counts the number of times the bus returns Data to the Data cache during read request */
	MRVL_PJ4B_DCACHE_WRITE_BEAT = 0x84, 	/* counts the number of times the bus returns ready to the Data cache during write request */
	MRVL_PJ4B_NO_DUAL_HW = 0x85, 	/* counts the number of cycles the processor cannot dual issue because of hardware conflict */
	MRVL_PJ4B_NO_DUAL_MULTIPLE = 0x86, 	/* counts the number of cycles the processor cannot dual issue because of multiple reasons */
	MRVL_PJ4B_BIU_ANY_ACCESS = 0x87, 	/* counts the number of cycles the BIU is accessed by any unit */
	MRVL_PJ4B_MAIN_TLB_REFILL_BY_ICACHE = 0x88, 	/* counts the number of instruction fetch operations that causes a Main TLB walk */
	MRVL_PJ4B_MAIN_TLB_REFILL_BY_DCACHE = 0x89, 	/* counts the number of Data read or write operations that causes a Main TLB walk */
	MRVL_PJ4B_ICACHE_READ_BEAT = 0x8a, 	/* counts the number of times the bus returns RDY to the instruction cache */
	MRVL_PJ4B_PMUEXT_IN0 = 0x90, 	/* counts any event from external input source PMUEXTIN[0] */
	MRVL_PJ4B_PMUEXT_IN1 = 0x91, 	/* counts any event from external input source PMUEXTIN[1] */
	MRVL_PJ4B_PMUEXT_IN0_IN1 = 0x92, 	/* counts any event from both external input sources PMUEXTIN[0] and PMUEXTIN[1] */
	MRVL_PJ4B_WMMX2_STORE_FIFO_FULL = 0xc0, 	/* counts the number of cycles when the WMMX2 store FIFO is full */
	MRVL_PJ4B_WMMX2_FINISH_FIFO_FULL = 0xc1, 	/* counts the number of cycles when the WMMX2 finish FIFO is full */
	MRVL_PJ4B_WMMX2_INST_FIFO_FULL = 0xc2, 	/* counts the number of cycles when the WMMX2 instruction FIFO is full */
	MRVL_PJ4B_WMMX2_INST_RETIRED = 0xc3, 	/* counts the number of retired WMMX2 instructions */
	MRVL_PJ4B_WMMX2_BUSY = 0xc4, 	/* counts the number of cycles when the WMMX2 is busy */
	MRVL_PJ4B_WMMX2_HOLD_MI = 0xc5, 	/* counts the number of cycles when WMMX2 holds the issue stage */
	MRVL_PJ4B_WMMX2_HOLD_MW = 0xc6, 	/* counts the number of cycles when WMMX2 holds the write back stage */
	/* EVT_CCNT is not hardware defined */
	MRVL_PJ4B_EVT_CCNT = 0xFE,		/* CPU_CYCLE */
	MRVL_PJ4B_EVT_UNUSED = 0xFF, 
};

enum  pj4b_pmu_counters {MRVL_PJ4B_CCNT=0, 
						MRVL_PJ4B_PMN0, 
						MRVL_PJ4B_PMN1, 
						MRVL_PJ4B_PMN2, 
						MRVL_PJ4B_PMN3, 
						MRVL_PJ4B_PMN4, 
						MRVL_PJ4B_PMN5, 
						MRVL_PJ4B_MAX_COUNTERS};

#define MRVL_PJ4B_CCNT_BIT_OFFSET 	31
#define MRVL_PJ4B_PMN_BIT_OFFSET 	0

#define MRVL_PJ4B_ALL_CNTRS			(0x8000003F)

/*
 * The hardware events that we support. We do support cache operations but
 * we have harvard caches and no way to combine instruction and data
 * accesses/misses in hardware.
 */
static const unsigned mrvl_pj4b_perf_map[PERF_COUNT_HW_MAX] = {
	[PERF_COUNT_HW_CPU_CYCLES]	    = MRVL_PJ4B_EVT_CCNT,
	[PERF_COUNT_HW_INSTRUCTIONS]	    = MRVL_PJ4B_INSN_EXECUTED,
	[PERF_COUNT_HW_CACHE_REFERENCES]    = HW_OP_UNSUPPORTED,
	[PERF_COUNT_HW_CACHE_MISSES]	    = HW_OP_UNSUPPORTED,
	[PERF_COUNT_HW_BRANCH_INSTRUCTIONS] = MRVL_PJ4B_BR_RETIRED,
	[PERF_COUNT_HW_BRANCH_MISSES]	    = MRVL_PJ4B_BR_PRED_MISS,
	[PERF_COUNT_HW_BUS_CYCLES]	    = HW_OP_UNSUPPORTED,
};

static const unsigned mrvl_pj4b_perf_cache_map[PERF_COUNT_HW_CACHE_MAX]
					[PERF_COUNT_HW_CACHE_OP_MAX]
					[PERF_COUNT_HW_CACHE_RESULT_MAX] = {
	[C(L1D)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]  = MRVL_PJ4B_DCACHE_ACCESS,
			[C(RESULT_MISS)]    = MRVL_PJ4B_DCACHE_READ_MISS,
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]  = MRVL_PJ4B_DCACHE_ACCESS,
			[C(RESULT_MISS)]    = MRVL_PJ4B_DCACHE_WRITE_MISS,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]  = CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]    = CACHE_OP_UNSUPPORTED,
		},
	},
	[C(L1I)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]  = CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]    = MRVL_PJ4B_ICACHE_READ_MISS,
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]  = CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]    = CACHE_OP_UNSUPPORTED,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]  = CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]    = CACHE_OP_UNSUPPORTED,
		},
	},
	/*TODO add L2 counters*/
	[C(LL)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]  = CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]    = CACHE_OP_UNSUPPORTED,
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]  = CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]    = CACHE_OP_UNSUPPORTED,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]  = CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]    = CACHE_OP_UNSUPPORTED,
		},
	},
	[C(DTLB)] = {
		/*
		 * The ARM performance counters can count micro DTLB misses,
		 * micro ITLB misses and main TLB misses. There isn't an event
		 * for TLB misses, so use the micro misses here and if users
		 * want the main TLB misses they can use a raw counter.
		 */
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]  = CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]    = MRVL_PJ4B_DTLB_MISS,
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]  = CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]    = MRVL_PJ4B_DTLB_MISS,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]  = CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]    = CACHE_OP_UNSUPPORTED,
		},
	},
	[C(ITLB)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]  = CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]    = MRVL_PJ4B_ITLB_MISS,
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]  = CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]    = MRVL_PJ4B_ITLB_MISS,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]  = CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]    = CACHE_OP_UNSUPPORTED,
		},
	},
	[C(BPU)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]  = MRVL_PJ4B_BR_RETIRED,
			[C(RESULT_MISS)]    = MRVL_PJ4B_BR_PRED_MISS,
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]  = MRVL_PJ4B_BR_RETIRED,
			[C(RESULT_MISS)]    = MRVL_PJ4B_BR_PRED_MISS,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]  = CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]    = CACHE_OP_UNSUPPORTED,
		},
	},
};

/*Helper functions*/
static inline void mrvl_pj4b_pmu_cntr_disable(u32 val)
{
	asm volatile("mcr p15, 0, %0, c9, c14, 2" : : "r"(val));
	asm volatile("mcr p15, 0, %0, c9, c12, 2" : : "r"(val));	
}

static inline void mrvl_pj4b_pmu_cntr_enable(u32 val)
{
	asm volatile("mcr p15, 0, %0, c9, c12, 1" : : "r"(val));	
	asm volatile("mcr p15, 0, %0, c9, c14, 1" : : "r"(val));
}

static inline void mrvl_pj4b_pmu_select_event(u32 cntr, u32 evt)
{
	asm volatile("mcr p15, 0, %0, c9, c12, 5" : : "r"(cntr));
	asm volatile("mcr p15, 0, %0, c9, c13, 1" : : "r"(evt));
}

static inline void mrvl_pj4b_pmu_clear_events(u32 val)
{
	asm volatile("mcr p15, 0, %0, c9, c12, 2" : : "r"(val));	
}

static inline void mrvl_pj4b_pmu_enable_events(u32 val)
{
	asm volatile("mcr p15, 0, %0, c9, c12, 1": : "r"(val));
}

static inline u32 mrvl_pj4b_pmu_read_events(void)
{
	u32 val;
	asm volatile("mcr p15, 0, %0, c9, c12, 1": "=r"(val));
	return val;
}


static inline void mrvl_pj4b_write_pmnc(u32 val)
{
	asm volatile("mcr p15, 0, %0, c9, c12, 0": : "r"(val));
} 

static inline u32 mrvl_pj4b_read_pmnc(void)
{
	u32 val;

	asm volatile("mrc p15, 0, %0, c9, c12, 0" : "=r"(val));

	return val;
}

static inline void mrvl_pj4b_pmu_clear_overflow(u32 val)
{
	/* writeback clears overflow bits */
	asm volatile("mcr p15, 0, %0, c9, c12, 3": : "r"(val));
}

static inline int mrvl_pj4b_pmu_counter_has_overflowed(unsigned long val,
				  enum pj4b_pmu_counters counter)
{
	int ret = 0;

	if (counter == MRVL_PJ4B_CCNT)
		ret = (val & (1 << MRVL_PJ4B_CCNT_BIT_OFFSET));
	else if (counter < MRVL_PJ4B_MAX_COUNTERS)
		ret = (val & (1 << (counter-MRVL_PJ4B_PMN0)));
	else
		WARN_ONCE(1, "invalid counter number (%d)\n", counter);

	return ret;

}

static inline u32 mrvl_pj4b_pmu_read_overflow(void)
{
	u32 val;
	/* check counter */
	asm volatile("mrc p15, 0, %0, c9, c12, 3" : "=r"(val));
	return val;
}


/*API functions*/
static u32 mrvl_pj4b_pmu_read_counter(int counter)
{
	u32 val = 0;
	
	switch (counter) {
	case MRVL_PJ4B_CCNT:
		asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r" (val));		
		break;
	case MRVL_PJ4B_PMN0:
	case MRVL_PJ4B_PMN1:
	case MRVL_PJ4B_PMN2:
	case MRVL_PJ4B_PMN3:
	case MRVL_PJ4B_PMN4:
	case MRVL_PJ4B_PMN5:
		asm volatile("mcr p15, 0, %0, c9, c12, 5": : "r"(counter - MRVL_PJ4B_PMN0));
		asm volatile("mrc p15, 0, %0, c9, c13, 2" : "=r"(val));
		break;
 	}	
	return val;
} 

static void mrvl_pj4b_pmu_write_counter(int counter, u32 val)
{
	switch (counter) {
	case MRVL_PJ4B_CCNT:
		asm volatile("mcr p15, 0, %0, c9, c13, 0" : : "r" (val));
		break;
	case MRVL_PJ4B_PMN0:
	case MRVL_PJ4B_PMN1:
	case MRVL_PJ4B_PMN2:
	case MRVL_PJ4B_PMN3:
	case MRVL_PJ4B_PMN4:
	case MRVL_PJ4B_PMN5:
		asm volatile("mcr p15, 0, %0, c9, c12, 5": : "r"(counter - MRVL_PJ4B_PMN0));
		asm volatile("mcr p15, 0, %0, c9, c13, 2": : "r"(val));
		break;
	}
}


static u64 mrvl_pj4b_pmu_raw_event(u64 config)
{
	return config & 0xff;
}

static inline int mrvl_pj4b_pmu_event_map(int config)
{
	int mapping = mrvl_pj4b_perf_map[config];
	if (HW_OP_UNSUPPORTED == mapping)
		mapping = -EOPNOTSUPP;
	return mapping;
}

static int mrvl_pj4b_pmu_get_event_idx(struct cpu_hw_events *cpuc,
				  struct hw_perf_event *event)
{
	int idx;
	/* Always place a cycle counter into the cycle counter. */
	if (event->config_base == MRVL_PJ4B_EVT_CCNT) {		
		if (test_and_set_bit(MRVL_PJ4B_CCNT, cpuc->used_mask)) {		
			return -EAGAIN;
		}
		return MRVL_PJ4B_CCNT;
	} else {
		/*
		 * For anything other than a cycle counter, try and use
		 * the events counters
		 */
		for (idx = MRVL_PJ4B_PMN0; idx < armpmu->num_events; ++idx) {
			if (!test_and_set_bit(idx, cpuc->used_mask)) {			
				return idx;
			}
		}
		/* The counters are all in use. */
		return -EAGAIN;
	}	
}

void mrvl_pj4b_pmu_enable_event(struct hw_perf_event *hwc, int idx)
{
	u32 enable;
	unsigned long flags;
	raw_spin_lock_irqsave(&pmu_lock, flags);
	if (idx == MRVL_PJ4B_CCNT) {	
		enable = (1 << MRVL_PJ4B_CCNT_BIT_OFFSET);
	} 
	else if (idx < MRVL_PJ4B_MAX_COUNTERS) {
		enable   = (1 << (idx - MRVL_PJ4B_PMN0));
	} 
	else {
		WARN_ONCE(1, "invalid counter number (%d)\n", idx);
		return;
	}
	mrvl_pj4b_pmu_cntr_disable(enable);
	/*select event*/
	if (idx != MRVL_PJ4B_CCNT) {		
		/*select event*/
		u32 evt = (hwc->config_base & 0xFF);		
		mrvl_pj4b_pmu_select_event((idx-MRVL_PJ4B_PMN0), evt);
	}	
	mrvl_pj4b_pmu_cntr_enable(enable);
	raw_spin_unlock_irqrestore(&pmu_lock, flags);	
}


void mrvl_pj4b_pmu_disable_event(struct hw_perf_event *hwc, int idx)
{
	u32 enable;
	unsigned long flags;
	raw_spin_lock_irqsave(&pmu_lock, flags);
	if (idx == MRVL_PJ4B_CCNT) {	
		enable = (1 << MRVL_PJ4B_CCNT_BIT_OFFSET);
	} 
	else if (idx < MRVL_PJ4B_MAX_COUNTERS) {
		enable   = (1 << (idx - MRVL_PJ4B_PMN0));
	} 
	else {
		WARN_ONCE(1, "invalid counter number (%d)\n", idx);
		return;
	}	
	mrvl_pj4b_pmu_cntr_disable(enable);
	raw_spin_unlock_irqrestore(&pmu_lock, flags);
}


static irqreturn_t mrvl_pj4b_pmu_handle_irq(int irq, void *arg)
{
	int i = 0;
	u32 flag;
	struct pt_regs *regs;
	struct perf_sample_data data;	
	struct cpu_hw_events *cpuc;	
	u32 pmnc;

	pmnc = mrvl_pj4b_read_pmnc();	
	pmnc &= ~MRVL_PJ4B_PMU_ENABLE;
	mrvl_pj4b_write_pmnc(pmnc);	

	flag = mrvl_pj4b_pmu_read_overflow();	
	mrvl_pj4b_pmu_clear_overflow(flag);

	/*
	 * Did an overflow occur?
	 */
	if (!flag) {		
		pmnc |= MRVL_PJ4B_PMU_ENABLE;
		mrvl_pj4b_write_pmnc(pmnc);
		return IRQ_NONE;
	}	
	/*
	 * Handle the counter(s) overflow(s)
	 */
	regs = get_irq_regs();
	perf_sample_data_init(&data, 0);

	cpuc = &__get_cpu_var(cpu_hw_events);

	for (i = MRVL_PJ4B_CCNT; i < armpmu->num_events; i++) {

		struct perf_event *event = cpuc->events[i];
		struct hw_perf_event *hwc;

		if (!test_bit(i, cpuc->active_mask)) {
			continue;
		}
		if (!mrvl_pj4b_pmu_counter_has_overflowed(flag, i)) {		
			continue;
		}

		hwc = &event->hw;
		armpmu_event_update(event, hwc, i,1);
		data.period = event->hw.last_period;

		if (!armpmu_event_set_period(event, hwc, i)) {		
			continue;
		}

		if (perf_event_overflow(event, 0, &data, regs))
			armpmu->disable(hwc, i);
 	}	
	pmnc |= MRVL_PJ4B_PMU_ENABLE;
	mrvl_pj4b_write_pmnc(pmnc);	
	
	/*
	 * Handle the pending perf events.
	 *
	 * Note: this call *must* be run with interrupts enabled. For
	 * platforms that can have the PMU interrupts raised as a PMI, this
	 * will not work.
	 */
	//perf_event_do_pending();
		irq_work_run();
	
	return IRQ_HANDLED;
} 



asmlinkage void __exception do_mrvl_pj4b_pmu_event(struct pt_regs *regs)
{	
	struct pt_regs *old_regs = set_irq_regs(regs);
	int cpu = smp_processor_id();
	irq_enter();
	irq_stat[cpu].local_pmu_irqs++;
	armpmu->handle_irq(IRQ_AURORA_MP, NULL);    
	irq_exit();	 	
	set_irq_regs(old_regs);
}


static void mrvl_pj4b_pmu_stop(void)
{
	u32 pmnc;
	unsigned long flags;
	raw_spin_lock_irqsave(&pmu_lock, flags);
	pmnc = mrvl_pj4b_read_pmnc();
	pmnc &= ~MRVL_PJ4B_PMU_ENABLE;
	mrvl_pj4b_write_pmnc(pmnc);	
	raw_spin_unlock_irqrestore(&pmu_lock, flags);
} 


static void mrvl_pj4b_pmu_start(void)
{
	u32 pmnc;
	unsigned long flags;

	raw_spin_lock_irqsave(&pmu_lock, flags);		
	pmnc = mrvl_pj4b_read_pmnc();
	pmnc |= (MRVL_PJ4B_PMU_ENABLE);
	mrvl_pj4b_write_pmnc(pmnc);
	raw_spin_unlock_irqrestore(&pmu_lock, flags);
} 

static u32 __init mrvl_pj4b_read_reset_pmnc(void)
{
	u32 pmnc = mrvl_pj4b_read_pmnc();
#ifdef CONFIG_SHEEVA_ERRATA_ARM_CPU_PMU_RESET
	mrvl_pj4b_write_pmnc(pmnc); /* WA - need to write 0 to bit 2 before
					write*/
#endif
	pmnc |= (MRVL_PJ4B_PMU_RESET);
	mrvl_pj4b_write_pmnc(pmnc);
	return ((pmnc >> 11) & 0x1F)+1;
}

static void mrvl_pj4b_pmu_reset(void *info)
{
	u32 idx, nb_cnt = armpmu->num_events;

	/* The counter and interrupt enable registers are unknown at reset. */
	for (idx = 1; idx < nb_cnt; ++idx)
		mrvl_pj4b_pmu_disable_event(NULL, idx);

	/* Initialize & Reset PMNC: C and P bits */
	mrvl_pj4b_write_pmnc(PJ4BV7_PMNC_P | PJ4BV7_PMNC_C);
	
}


static const struct arm_pmu mrvl_pj4b_pmu = {
	
	.id				= MRVL_PERF_PMU_ID_PJ4B,
	.name			= "Armada PJ4",
	.stop			= mrvl_pj4b_pmu_stop, /*v*/
	.start			= mrvl_pj4b_pmu_start, /*v*/
	.enable			= mrvl_pj4b_pmu_enable_event, /*v*/
	.disable		= mrvl_pj4b_pmu_disable_event, /*v*/
	.read_counter	= mrvl_pj4b_pmu_read_counter, /*v*/
	.write_counter	= mrvl_pj4b_pmu_write_counter, /*v*/
	.get_event_idx	= mrvl_pj4b_pmu_get_event_idx,/*v*/
	.cache_map		= &mrvl_pj4b_perf_cache_map,
	.event_map		= &mrvl_pj4b_perf_map, //mrvl_pj4b_pmu_event_map, /*v*/
	.handle_irq		= mrvl_pj4b_pmu_handle_irq, /*v*/
	.reset 			= mrvl_pj4b_pmu_reset,
	/*.raw_event		= mrvl_pj4b_pmu_raw_event,*/
	.raw_event_mask	= 0xFF,
	.num_events		= MRVL_PJ4B_MAX_COUNTERS,
	.max_period		= (1LLU << 32) - 1,
};

static const struct arm_pmu *__init mrvl_pj4b_pmu_init(void)
{
	return &mrvl_pj4b_pmu;
}

#endif /*#ifdef CONFIG_ARCH_ARMADA_XP*/
