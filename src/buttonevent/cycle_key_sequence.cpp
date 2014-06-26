/*
**  Xbox360 USB Gamepad Userspace Driver
**  Copyright (C) 2011 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "buttonevent/cycle_key_sequence.hpp"

#include <stdexcept>

#include "raise_exception.hpp"

CycleKeySequencePtr
CycleKeySequence::from_range(UInput& uinput, int slot, bool extra_devices,
                             std::vector<std::string>::const_iterator beg,
                             std::vector<std::string>::const_iterator end, 
                             bool wrap_around)
{
  Keys keys;

  for(std::vector<std::string>::const_iterator i = beg; i != end; ++i)
  {
    keys.push_back(UIEventSequence::from_string(*i));
  }

  if (keys.empty())
  {
    raise_exception(std::runtime_error, "no keys found");
  }
  else
  {
    return CycleKeySequencePtr(new CycleKeySequence(uinput, slot, extra_devices, 
                                                    keys, wrap_around));
  }
}

CycleKeySequence::CycleKeySequence(UInput& uinput, int slot, bool extra_devices,
                                   const Keys& keys, bool wrap_around) :
  m_keys(keys),
  m_wrap_around(wrap_around),
  m_current_key(0),
  m_last_key(0),
  m_pressed(-1)
{
  assert(!m_keys.empty());

  for(Keys::iterator i = m_keys.begin(); i != m_keys.end(); ++i)
  {
    i->init(uinput, slot, extra_devices);
  }
}

void
CycleKeySequence::send(bool value)
{
  int send_key = has_current_key() ? m_current_key : m_last_key;

  if (m_pressed != -1) // is_pressed()
  {
    // if one key in the sequence is pressed and we receive another
    // press event, release the last pressed key
    if (value)
    {
      // release the last key
      m_keys[m_pressed].send(uinput, false); 

      // press the new key
      m_keys[send_key].send(uinput, value); 

      // record it for later use
      m_pressed = send_key;
    }
    else  // (!value)
    {
      if (send_key == m_pressed)
      {
        // pressed key is released
        m_keys[send_key].send(uinput, value); 
        m_pressed = -1;
      }
      else
      {
        // FIXME: does not work, need a way to distinguish who is sending the
        // release event
      }
    }
  }
  else
  {
    // no key is currently pressed
    m_keys[send_key].send(uinput, value); 
    if (value)
    {
      // record the last key press event send for potential later release
      m_pressed = send_key;
    }
  }

  m_last_key = send_key;
  m_current_key = -1;
}

void
CycleKeySequence::next_key()
{
  if (has_current_key())
  {
    if (m_current_key == static_cast<int>(m_keys.size() - 1))
    {
      if (m_wrap_around)
      {
        m_current_key = 0;
      }
    }
    else
    {
      m_current_key += 1;
    }
  }
  else
  {
    m_current_key = m_last_key;
    next_key();
  }
}

void
CycleKeySequence::prev_key()
{
  if (has_current_key())
  {
    if (m_current_key == 0)
    {
      if (m_wrap_around)
      {
        m_current_key = static_cast<int>(m_keys.size() - 1);
      }
    }
    else
    {
      m_current_key -= 1;
    }
  }
  else
  {
    m_current_key = m_last_key;
    prev_key();
  }
}

/* EOF */
