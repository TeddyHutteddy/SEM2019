/*
 * module.cpp
 *
 * Created: 10/12/2018 2:40:06 PM
 *  Author: teddy
 */ 

#include <string.h>

#include "module.h"

rt::twi::RegisterDesc rt::module::metadata::horn::RegMetadata[] = {
	//write, regular, next, len, [pos]
	{false, false, false, 2}, //Header
	{false, false, false, 1}, //Signature
	{false, false, false, 1}, //ID
	{false, false, false, metadata::com::NameLength}, //Name
	{false, true, false, 1}, //Status
	{true, false, false, 1}  //Settings
};

rt::twi::ModuleRegMeta rt::module::metadata::horn::TWIDescriptor{RegMetadata, sizeof RegMetadata / sizeof(rt::twi::RegisterDesc)};

uint8_t rt::twi::ModuleScanner::found() const
{
	//If found, return address and found flag
	if(pm_state == State::Found)
		return 0x80 | pm_currentaddress;
	//If not found, but still scanning return 0x00
	if(pm_state == State::Scanning || pm_state == State::Reading)
		return 0x00;
	//If idle returns 0x7f
	return 0x7f;
}

rt::twi::ModuleDescriptor rt::twi::ModuleScanner::foundModule() const
{
	return pm_descriptor;
}

void rt::twi::ModuleScanner::update()
{
	using namespace module;
	switch(pm_state) {
	case State::Idle:
	case State::Found:
		//This really should be set in the member function "scan", but trying to save space by not duplicating the inherited one
		if(!pm_scanning)
			break;
		pm_state = State::Scanning;
		//[[fallthrough]];
	case State::Scanning:
		TWIScanner::update();
		//If a device was found on the TWI bus, and reads 0x5E (for module), then read from register address 0 header, signature and id
		if(pm_found && readbuf[0] == metadata::com::Header[0]) {
			twimaster.readFromAddress(pm_currentaddress, 0x00, readbuf, sizeof readbuf);
			pm_state = State::Reading;
		}
		break;
	case State::Reading:
		if(twimaster.attention()) {
			//Check that it was a successful read, and the header is correct
			if(twimaster.result() == hw::TWIMaster::Result::Success &&
			   memcmp(readbuf, metadata::com::Header, sizeof metadata::com::Header) == 0) {
				//Read signature and id
				pm_descriptor.addr = pm_currentaddress;
				pm_descriptor.signature = readbuf[metadata::com::offset::Signature];
				pm_descriptor.id = readbuf[metadata::com::offset::ID];
				pm_state = State::Found;
			}
			//If not, resume scanning
			else {
				//Note: Potential for glitch here: Will now only scan from currentaddress to endaddress, meaning if oneshot is disabled
				//the start addresses to be scanned are truncated in subsequent loops
				scan(pm_currentaddress, pm_endaddress, pm_oneshot);
				pm_state = State::Scanning;
			}
		}
		break;
	}
}

rt::twi::ModuleScanner::ModuleScanner(hw::TWIMaster &twimaster) : TWIScanner(twimaster) {}

void rt::twi::ModuleScanner::addressCheck(uint8_t const addr)
{
	readbuf[0] = 0;
	//Read to check for 5E (will be the first byte sent by a module every time)
	twimaster.readBuffer(addr, readbuf, 1);
}

uint8_t rt::module::Master::get_signature() const
{
	return buffer.serialiseRead<uint8_t>(metadata::com::offset::Signature);
}

uint8_t rt::module::Master::get_id() const
{
	return buffer.serialiseRead<uint8_t>(metadata::com::offset::ID);
}

char const * rt::module::Master::get_name() const
{
	return reinterpret_cast<char const *>(buffer.pm_ptr + metadata::com::offset::Name);
}

bool rt::module::Master::get_active() const
{
	return buffer.bitGet(metadata::com::offset::Status, metadata::com::sig::status::Active);
}

bool rt::module::Master::get_operational() const
{
	return buffer.bitGet(metadata::com::offset::Status, metadata::com::sig::status::Operational);
}

void rt::module::Master::set_led(bool const state)
{
	buffer.bitSet(metadata::com::offset::Settings, metadata::com::sig::settings::LED, state);
}

void rt::module::Master::set_power(bool const state)
{
	buffer.bitSet(metadata::com::offset::Settings, metadata::com::sig::settings::Power, state);
}

void rt::module::Master::update()
{
	buffermanager.update();
}

rt::module::Master::Master(hw::TWIMaster &twimaster, uint8_t const twiaddr, utility::Buffer &buffer, twi::ModuleRegMeta const &regs, size_t const updateInterval)
 : buffer(buffer), buffermanager(twimaster, twiaddr, buffer, regs, updateInterval, 1) {}

void rt::module::Horn::set_state(bool const state)
{
	buffer.bitSet(metadata::com::offset::Settings, metadata::horn::sig::settings::HornState, state);
}

 rt::module::Horn::Horn(hw::TWIMaster &twimaster, uint8_t const twiaddr, size_t const updateInterval /*= 1000 / 30*/)
  : Master(twimaster, twiaddr, buffer, metadata::horn::TWIDescriptor, updateInterval)
{
	buffermanager.run();
}
