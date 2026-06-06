#include <timer.h>
#include <isr.h> // pentru înregistrarea handlerului de întrerupere

volatile uint32_t tick = 0;

void timer_handler(registers_t *regs)
{
    tick++;
    // Apelează scheduler-ul pentru preempțiune
    // schedule();
}

void timer_init(uint32_t frequency)
{
    uint32_t divisor = 1193180 / frequency;
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
    register_interrupt_handler(32, timer_handler); // IRQ0
}