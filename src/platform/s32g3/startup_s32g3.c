/*
 * startup_s32g3.c
 * NXP S32G3 Startup Code
 * ARM Cortex-A53 Core Initialization
 */

#include <stdint.h>
#include <stddef.h>

/*==============================================================================
 * S32G3 Memory Map
 *============================================================================*/

/* SRAM Regions */
#define SRAM0_BASE              0x34000000UL
#define SRAM0_SIZE              0x00800000UL    /* 8 MB */

/* DDR Memory */
#define DDR_BASE                0x80000000UL
#define DDR_SIZE                0x80000000UL    /* 2 GB */

/* Peripheral Base */
#define PERIPHERAL_BASE         0x40000000UL

/* NVIC/GIC Base */
#define GIC_BASE                0x50800000UL
#define GICD_BASE               (GIC_BASE + 0x1000)
#define GICC_BASE               (GIC_BASE + 0x2000)

/*==============================================================================
 * Stack Configuration
 *============================================================================*/

/* Stack sizes for different processor modes */
#define STACK_TOP               0x34200000UL    /* Top of SRAM0 */
#define STACK_SIZE_IRQ          0x00010000UL    /* 64 KB IRQ stack */
#define STACK_SIZE_FIQ          0x00010000UL    /* 64 KB FIQ stack */
#define STACK_SIZE_ABORT        0x00010000UL    /* 64 KB Abort stack */
#define STACK_SIZE_UNDEFINED    0x00010000UL    /* 64 KB Undefined stack */
#define STACK_SIZE_SYSTEM       0x00040000UL    /* 256 KB System stack */

/*==============================================================================
 * External Symbols
 *============================================================================*/

/* Linker script defined symbols */
extern uint32_t __text_start__;
extern uint32_t __text_end__;
extern uint32_t __rodata_start__;
extern uint32_t __rodata_end__;
extern uint32_t __data_start__;
extern uint32_t __data_end__;
extern uint32_t __data_load__;
extern uint32_t __bss_start__;
extern uint32_t __bss_end__;
extern uint32_t __stack_top__;

/* Main application entry */
extern int main(void);

/* C runtime initialization */
extern void __libc_init_array(void);

/*==============================================================================
 * Default Exception Handlers
 *============================================================================*/

/* Weak attribute allows override by application */
#define WEAK __attribute__((weak))

void WEAK Reset_Handler(void);
void WEAK Undefined_Handler(void);
void WEAK SVC_Handler(void);
void WEAK PrefetchAbort_Handler(void);
void WEAK DataAbort_Handler(void);
void WEAK IRQ_Handler(void);
void WEAK FIQ_Handler(void);

/* Default handler - infinite loop */
static void Default_Handler(void)
{
    while (1) {
        __asm__ volatile ("wfi");
    }
}

/* Exception handlers default to infinite loop */
void Undefined_Handler(void)      { Default_Handler(); }
void SVC_Handler(void)            { Default_Handler(); }
void PrefetchAbort_Handler(void)  { Default_Handler(); }
void DataAbort_Handler(void)      { Default_Handler(); }
void IRQ_Handler(void)            { Default_Handler(); }
void FIQ_Handler(void)            { Default_Handler(); }

/*==============================================================================
 * Interrupt Vector Table
 * ARMv8-A Vector Table Format (each entry 128 bytes)
 *============================================================================*/

__attribute__((section(".vectors"), aligned(2048)))
void (* const vector_table[])(void) = {
    /* Current EL with SP0 */
    (void (*)(void))STACK_TOP,      /* SP_EL0 - not used */
    Reset_Handler,                   /* Reset */
    Undefined_Handler,               /* Undefined Instruction */
    Default_Handler,                 /* SVC */
    PrefetchAbort_Handler,           /* Prefetch Abort */
    DataAbort_Handler,               /* Data Abort */
    Default_Handler,                 /* Hyp Entry */
    IRQ_Handler,                     /* IRQ */
    FIQ_Handler,                     /* FIQ */
    
    /* Current EL with SPx - same handlers */
    [16 ... 31] = Default_Handler,
    
    /* Lower EL using AArch64 - not used */
    [32 ... 47] = Default_Handler,
    
    /* Lower EL using AArch32 - not used */
    [48 ... 63] = Default_Handler,
};

/*==============================================================================
 * System Register Access Helpers
 *============================================================================*/

/* Write Vector Base Address Register */
static inline void set_vbar(uint64_t addr)
{
    __asm__ volatile ("msr vbar_el1, %0" :: "r"(addr));
}

/* Read Multiprocessor Affinity Register */
static inline uint64_t get_mpidr(void)
{
    uint64_t mpidr;
    __asm__ volatile ("mrs %0, mpidr_el1" : "=r"(mpidr));
    return mpidr;
}

/* Read Current EL */
static inline uint32_t get_current_el(void)
{
    uint64_t el;
    __asm__ volatile ("mrs %0, currentel" : "=r"(el));
    return (uint32_t)((el >> 2) & 0x3);
}

/* Data Synchronization Barrier */
static inline void dsb(void)
{
    __asm__ volatile ("dsb sy" ::: "memory");
}

/* Instruction Synchronization Barrier */
static inline void isb(void)
{
    __asm__ volatile ("isb" ::: "memory");
}

/* Wait For Interrupt */
static inline void wfi(void)
{
    __asm__ volatile ("wfi");
}

/*==============================================================================
 * GIC (Generic Interrupt Controller) Initialization
 *============================================================================*/

/* GIC Distributor Registers */
#define GICD_CTLR       (*((volatile uint32_t *)(GICD_BASE + 0x000)))
#define GICD_TYPER      (*((volatile uint32_t *)(GICD_BASE + 0x004)))
#define GICD_IGROUPR(n) (*((volatile uint32_t *)(GICD_BASE + 0x080 + (n) * 4)))
#define GICD_ISENABLER(n) (*((volatile uint32_t *)(GICD_BASE + 0x100 + (n) * 4)))
#define GICD_ICENABLER(n) (*((volatile uint32_t *)(GICD_BASE + 0x180 + (n) * 4)))
#define GICD_ISPENDR(n) (*((volatile uint32_t *)(GICD_BASE + 0x200 + (n) * 4)))
#define GICD_ICPENDR(n) (*((volatile uint32_t *)(GICD_BASE + 0x280 + (n) * 4)))
#define GICD_ICFGR(n)   (*((volatile uint32_t *)(GICD_BASE + 0xC00 + (n) * 4)))

/* GIC CPU Interface Registers */
#define GICC_CTLR       (*((volatile uint32_t *)(GICC_BASE + 0x000)))
#define GICC_PMR        (*((volatile uint32_t *)(GICC_BASE + 0x004)))
#define GICC_BPR        (*((volatile uint32_t *)(GICC_BASE + 0x008)))

/* GIC Control Bits */
#define GICD_CTLR_ENABLE    (1UL << 0)
#define GICC_CTLR_ENABLE    (1UL << 0)

/*
 * gic_init - Initialize Generic Interrupt Controller
 */
static void gic_init(void)
{
    uint32_t i;
    uint32_t num_irqs;
    
    /* Read number of interrupt lines */
    num_irqs = ((GICD_TYPER & 0x1F) + 1) * 32;
    
    /* Disable distributor */
    GICD_CTLR = 0;
    dsb();
    
    /* Configure all interrupts as Group 1 (IRQ) */
    for (i = 0; i < (num_irqs / 32); i++) {
        GICD_IGROUPR(i) = 0xFFFFFFFF;
    }
    
    /* Disable all interrupts */
    for (i = 0; i < (num_irqs / 32); i++) {
        GICD_ICENABLER(i) = 0xFFFFFFFF;
    }
    
    /* Clear all pending interrupts */
    for (i = 0; i < (num_irqs / 32); i++) {
        GICD_ICPENDR(i) = 0xFFFFFFFF;
    }
    
    /* Configure all interrupts as level-sensitive */
    for (i = 0; i < (num_irqs / 16); i++) {
        GICD_ICFGR(i) = 0;
    }
    
    /* Enable distributor */
    GICD_CTLR = GICD_CTLR_ENABLE;
    dsb();
    
    /* Configure CPU interface */
    GICC_PMR = 0xFF;        /* Priority mask - allow all */
    GICC_BPR = 0x0;         /* Binary point */
    
    /* Enable CPU interface */
    GICC_CTLR = GICC_CTLR_ENABLE;
    dsb();
    isb();
}

/*==============================================================================
 * Data/BSS Initialization
 *============================================================================*/

/*
 * init_data_section - Copy initialized data from flash to RAM
 */
static void init_data_section(void)
{
    uint32_t *src;
    uint32_t *dst;
    uint32_t *end;
    
    src = &__data_load__;
    dst = &__data_start__;
    end = &__data_end__;
    
    while (dst < end) {
        *dst++ = *src++;
    }
}

/*
 * init_bss_section - Zero initialize BSS section
 */
static void init_bss_section(void)
{
    uint32_t *dst;
    uint32_t *end;
    
    dst = &__bss_start__;
    end = &__bss_end__;
    
    while (dst < end) {
        *dst++ = 0;
    }
}

/*==============================================================================
 * Cache Operations
 *============================================================================*/

/* Enable I-cache and D-cache */
static void enable_caches(void)
{
    uint64_t sctlr;
    
    /* Read System Control Register */
    __asm__ volatile ("mrs %0, sctlr_el1" : "=r"(sctlr));
    
    /* Enable I-cache (bit 12) and D-cache (bit 2) */
    sctlr |= (1UL << 12) | (1UL << 2);
    
    /* Write back */
    __asm__ volatile ("msr sctlr_el1, %0" :: "r"(sctlr));
    isb();
}

/*==============================================================================
 * Main Reset Handler
 *============================================================================*/

void Reset_Handler(void)
{
    uint64_t mpidr;
    uint32_t core_id;
    
    /* Get core ID */
    mpidr = get_mpidr();
    core_id = (uint32_t)(mpidr & 0xFF);
    
    /* Only core 0 performs system initialization */
    if (core_id == 0) {
        /* Set up vector table */
        set_vbar((uint64_t)vector_table);
        isb();
        
        /* Initialize data and bss sections */
        init_data_section();
        init_bss_section();
        
        /* Enable caches */
        enable_caches();
        
        /* Initialize interrupt controller */
        gic_init();
        
        /* Call C++ static constructors */
        __libc_init_array();
        
        /* Jump to main application */
        main();
        
        /* If main returns, enter infinite loop */
        while (1) {
            wfi();
        }
    } else {
        /* Secondary cores - enter WFI waiting for SMP boot */
        while (1) {
            wfi();
            /* Wake up handler would set core entry point and release */
        }
    }
}

/*==============================================================================
 * System Tick Timer (optional - for bare-metal timing)
 *============================================================================*/

/* System Counter (ARM Generic Timer) */
#define CNTFRQ          50000000UL      /* 50 MHz typical */

/*
 * get_system_tick - Get current system tick count
 */
uint64_t get_system_tick(void)
{
    uint64_t cnt;
    __asm__ volatile ("mrs %0, cntpct_el0" : "=r"(cnt));
    return cnt;
}

/*
 * get_system_freq - Get system counter frequency
 */
uint32_t get_system_freq(void)
{
    uint64_t freq;
    __asm__ volatile ("mrs %0, cntfrq_el0" : "=r"(freq));
    return (uint32_t)freq;
}

/*
 * delay_ms - Simple busy-wait delay
 */
void delay_ms(uint32_t ms)
{
    uint64_t start;
    uint64_t end;
    uint64_t ticks;
    
    start = get_system_tick();
    ticks = ((uint64_t)ms * get_system_freq()) / 1000;
    end = start + ticks;
    
    while (get_system_tick() < end) {
        /* Busy wait */
    }
}

/*==============================================================================
 * Error Handlers
 *============================================================================*/

/*
 * _exit - Called when application exits
 */
void _exit(int status)
{
    (void)status;
    while (1) {
        wfi();
    }
}

/*
 * _sbrk - Heap extension (minimal implementation)
 */
extern char _end;  /* End of BSS, start of heap */
static char *heap_end = NULL;

void *_sbrk(intptr_t increment)
{
    char *prev_heap_end;
    char *new_heap_end;
    
    if (heap_end == NULL) {
        heap_end = &_end;
    }
    
    prev_heap_end = heap_end;
    new_heap_end = heap_end + increment;
    
    /* Check heap limit (simplified - would check against stack in real impl) */
    heap_end = new_heap_end;
    
    return (void *)prev_heap_end;
}

/*
 * Default handler for unimplemented C library functions
 */
int _write(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    return len;
}

int _close(int file)
{
    (void)file;
    return -1;
}

int _fstat(int file, void *st)
{
    (void)file;
    (void)st;
    return 0;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

int _lseek(int file, int ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

int _read(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;
    return 0;
}
