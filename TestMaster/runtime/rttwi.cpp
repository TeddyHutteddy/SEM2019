/*
 * rttwi.cpp
 *
 * Created: 5/01/2019 3:42:38 PM
 *  Author: teddy
 */ 

#include <stdlib.h>

#include "rttwi.h"

void rt::twi::TWIScanner::scan(uint8_t const startaddress, uint8_t const endaddress, bool const oneshot, uint8_t const startfrom)
{
	pm_startaddress = startaddress;
	pm_endaddress = endaddress;
	pm_currentaddress = (startfrom == 0xff ? startaddress : startfrom);
	pm_oneshot = oneshot;
	pm_found = false;
	pm_scanning = true;
	addressCheck(startaddress);
}

uint8_t rt::twi::TWIScanner::found() const
{
	//If found, return address and found flag
	if(pm_found)
		return 0x80 | pm_currentaddress;
	//If not found, but still scanning return 0x00
	if(pm_scanning)
		return 0x00;
	//If not found and not scanning returns 7f
	return 0x7f;
}

void rt::twi::TWIScanner::update()
{
	if(pm_scanning && twimaster.attention()) {
		//If the operation was a success, a device was found
		if(twimaster.result() == hw::TWIMaster::Result::Success) {
			pm_found = true;
			pm_scanning = false;
		}
		//Device was not found
		else {
			if(++pm_currentaddress >= pm_endaddress) {
				if(pm_oneshot)
				//found should already be false
				pm_scanning = false;
				else
				pm_currentaddress = pm_startaddress;
			}
			addressCheck(pm_currentaddress);
		}
	}
}

rt::twi::TWIScanner::TWIScanner(hw::TWIMaster &twimaster) : twimaster(twimaster)
{
	pm_found = false;
	pm_scanning = false;
}

void rt::twi::TWIScanner::addressCheck(uint8_t const addr)
{
	twimaster.checkForAddress(addr);
}

void rt::twi::ModuleRegMeta::processPositions() const
{
	//See rttwi.h for information
	for(uint8_t i = 0; i < count; i++) {
		if(regs[i].pos == 0xff) {
			if(i == 0) regs[i].pos = 0;
			else
				regs[i].pos = regs[i - 1].pos + regs[i - 1].len;
		}
	}
}

void rt::twi::ModuleRegMeta::allNextUpdate() const
{
	for(uint8_t i = 0; i < count; i++) {
		regs[i].nextUpdate = true;
	}
}

rt::twi::MasterBufferManager::MasterBufferManager(hw::TWIMaster &twimaster, uint8_t const twiaddr, libmodule::utility::Buffer &buffer, ModuleRegMeta const &regs, size_t const updateInterval /*= 1000 / 30*/, uint8_t const headerCount /*= 0*/, bool const run)
 : twimaster(twimaster), m_twiaddr(twiaddr), buffer(buffer), m_regs(regs), m_updateInterval(updateInterval), m_headersize(headerCount)
{
	m_consecutiveCycleErrors = 0;
	pm_cycleError = false;
	//buffer.m_callbacks = this;
	m_regs.processPositions();
	m_regs.allNextUpdate();
	if(run)
		this->run();
}

uint8_t rt::twi::MasterBufferManager::findRegIndexFromBufferPos(size_t const pos) const
{
	for(uint8_t i = 0; i < m_regs.count; i++) {
		if(m_regs.regs[i].pos >= pos)
			return i;
	}
	return 0xff;
}

uint8_t rt::twi::MasterBufferManager::findRegIndexOfLastSimilar(uint8_t const regpos) const
{
	//If last position, can't go further
	if(regpos + 1 >= m_regs.count) return m_regs.count;

	for(uint8_t i = regpos + 1; i < m_regs.count; i++) {
		auto &primaryreg = m_regs.regs[i - 1];
		auto &secondaryreg = m_regs.regs[i];
		//Check that both are the same direction, are adjacent in the buffer, and will be updated this cycle
		if(secondaryreg.write == primaryreg.write
		&& secondaryreg.pos == primaryreg.pos + primaryreg.len
		&& (secondaryreg.queueUpdate || secondaryreg.nextUpdate || secondaryreg.regularUpdate))
			continue;
		//If these conditions are not met, return i
		return i;
	}
	//All until the end were valid, return count (the end)
	return m_regs.count;
}

volatile hw::TWIMaster::Result mres;

void rt::twi::MasterBufferManager::update()
{
	//Cannot do this in constructor because buffer constructor runs after, setting it back to nullptr
	buffer.m_callbacks = this;
	//A new cycle
	if(pm_timer && pm_state != State::Off) {
		pm_state = State::ProcessingCycle;
		pm_cycleError = false;
		pm_regindex_complete = 0;
		pm_regindex_attempted = 0;
		pm_timer = m_updateInterval;
		pm_timer.start();
	}
	//If the TWIMaster has finished an operation
	if(twimaster.attention()) {
		//If it was reading, copy the result into the client buffer
		if(pm_currentTransaction == TransactionType::Read) {
			if(twimaster.result() == hw::TWIMaster::Result::Success) {
				//Copy readbuf into buffer
				memcpy(buffer.pm_ptr + pm_readbuf.bufferoffset, pm_readbuf.buf + m_headersize, pm_readbuf.len - m_headersize);
			}
			//This is freed further down
			//free(pm_readbuf.buf);
			//pm_readbuf.buf = nullptr;
		}
		//Since the operation is finished, set TransactionType back to none
		pm_currentTransaction = TransactionType::None;
		//If the cycle is still processing (more regs to do)
		if(pm_state == State::ProcessingCycle) {
			mres = twimaster.result();
			//If there was no error
			if(mres == hw::TWIMaster::Result::Success) {
				//For the elements covered, set 'NextUpdate' to false (is set during writeCallback or during first transaction)
				for(uint8_t i = pm_regindex_complete; i < pm_regindex_attempted; i++) {
					m_regs.regs[i].queueUpdate = false;
				}
				pm_regindex_complete = pm_regindex_attempted;
			}
			//If there was an error this cycle, add to the consecutiveCycleError count
			else if(!pm_cycleError) {
				m_consecutiveCycleErrors++;
				pm_cycleError = true;
			}
			//Keep cycling from the successfully completed registers until a register to be read or written is found
			for(pm_regindex_attempted = pm_regindex_complete; pm_regindex_attempted < m_regs.count; pm_regindex_attempted++) {
				auto &element = m_regs.regs[pm_regindex_attempted];
				//If this element is to be updated, break
				if(element.queueUpdate || element.nextUpdate || element.regularUpdate)
					break;
			}
			//If nothing was found, move onto the next cycle
			if(pm_regindex_attempted >= m_regs.count) {
				pm_state = State::Waiting;
				//If there hasn't been an error this cycle, reset consecutive errors
				if(!pm_cycleError)
					m_consecutiveCycleErrors = 0;
				return;
			}
			//From here on, a read or write will occur
			auto &element = m_regs.regs[pm_regindex_attempted];
			//Find whether there are multiple similar regs in series (several birds with one stone)
			auto secondary_pos = findRegIndexOfLastSimilar(pm_regindex_attempted);
			//Queue these regs for update and set nextupdate to false
			for(uint8_t i = pm_regindex_attempted; i < secondary_pos; i++) {
				m_regs.regs[i].nextUpdate = false;
				m_regs.regs[i].queueUpdate = true;
			}
			pm_regindex_attempted = secondary_pos;
			//Create secondary element for convenience (above func gives past the end pos)
			auto &secondaryelement = m_regs.regs[pm_regindex_attempted - 1];
			//Size of buffer needed for transaction
			uint8_t bufferSize = secondaryelement.pos + secondaryelement.len - element.pos;
			//Write
			if(element.write) {
				//Make sure that the end of the transaction is not past the end of the buffer
				if(element.pos + bufferSize <= buffer.pm_len) {
					pm_currentTransaction = TransactionType::Write;
					//Should really properly check that this is a valid transfer to the buffer (same with for read)
					twimaster.writeToAddress(m_twiaddr, element.pos, buffer.pm_ptr + element.pos, bufferSize);

				}
			}
			//Read
			else {
				pm_currentTransaction = TransactionType::Read;
				//Allocate new memory for read, since the header needs to be cut off when transferring into client buffer
				pm_readbuf.len = bufferSize + m_headersize;
				pm_readbuf.bufferoffset = element.pos;
				if(pm_readbuf.buf != nullptr) free(pm_readbuf.buf);
				pm_readbuf.buf = static_cast<uint8_t *>(malloc(pm_readbuf.len));
				if(pm_readbuf.buf == nullptr) libmodule::hw::panic();
				twimaster.readFromAddress(m_twiaddr, element.pos, pm_readbuf.buf, pm_readbuf.len);
			}
		}
	}
	//If the cycle is not processing, the buffer has changed, and there is not communication in progress
	if(pm_state != State::ProcessingCycle && m_new_buffer.pm_ptr != nullptr && !twimaster.communicating()) {
		buffer.pm_ptr = m_new_buffer.pm_ptr;
		buffer.pm_len = m_new_buffer.pm_len;
		m_new_buffer.pm_ptr = nullptr;
	}
}

void rt::twi::MasterBufferManager::buffer_writeCallack(void const *const buf, size_t const len, size_t const pos)
{
	//If a register was written to, update it on the next cycle
	for(auto regPos = findRegIndexFromBufferPos(pos); m_regs.regs[regPos].pos <= pos + len && regPos < m_regs.count; regPos++) {
		auto &element = m_regs.regs[regPos];
		if(element.write)
			element.nextUpdate = true;
	}
}

void rt::twi::MasterBufferManager::buffer_readCallback(void *const buf, size_t const len, size_t const pos)
{
	//Since this is basically a copy from the top, make some sort of templated std::for_each sort of thing
	//(but customised for the conditions... need a predicate function with the conditions or something)
	
	//Doh, just don't do anything here
}

void rt::twi::MasterBufferManager::run()
{
	pm_state = State::Waiting;
	pm_timer.finished = true;
}

void rt::twi::MasterBufferManager::stop()
{
	pm_state = State::Off;
}
