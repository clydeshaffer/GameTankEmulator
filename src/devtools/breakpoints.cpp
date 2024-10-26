#include "breakpoints.h"
#include <algorithm>
int Breakpoints::breakCooldown = 0;
bool Breakpoints::enabled = false;
vector<Breakpoint> Breakpoints::breakpoints;

#define BREAKPOINT_COOLDOWN 16

bool Breakpoints::checkBreakpoint(uint16_t address, uint8_t bank) {
    if(!enabled) return false;
    if(breakCooldown) {
        --breakCooldown;
        return false;
    }
    //for some reason it was getting stuck so wait a number of cycles between breakpoint activations

    auto matcher = [address, bank](Breakpoint i) { return (i.address == address) && (!i.bank_set || (i.bank == bank)) && i.enabled && !i.linkFailed; };
    auto it = std::find_if(begin(breakpoints), end(breakpoints), matcher);

    if(it != std::end(breakpoints)) {
        breakCooldown = BREAKPOINT_COOLDOWN;
    }
    return breakCooldown == BREAKPOINT_COOLDOWN;;
}

//Scan memory map for 
void Breakpoints::linkBreakpoints(MemoryMap& memorymap) {
    for(auto& bp : breakpoints) {
        if(!bp.by_address) {
            if((!bp.linked) && (!bp.linkFailed)) {
                if(memorymap.FindName(bp.address, bp.name)) {
                    bp.linked = true;
                } else {
                    bp.linkFailed = true;
                }
            }
        }
    }
}