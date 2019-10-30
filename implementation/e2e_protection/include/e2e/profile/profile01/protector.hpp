// Copyright (C) 2014-2017 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef VSOMEIP_E2E_PROFILE01_PROTECTOR_HPP
#define VSOMEIP_E2E_PROFILE01_PROTECTOR_HPP

#include <boost/thread.hpp>
#include "../profile01/profile_01.hpp"
#include "../profile_interface/protector.hpp"

namespace vsomeip {
namespace e2e {
namespace profile01 {

class protector final : public e2e::profile_interface::protector {
  public:
    protector(void) = delete;

    explicit protector(const profile_config &_config) : config_(_config), counter_(0){};

    void protect(e2e_buffer &_buffer) override final;

  private:

    void write_counter(e2e_buffer &_buffer);

    void write_data_id(e2e_buffer &_buffer);

    void write_crc(e2e_buffer &_buffer, uint8_t _computed_crc);

    void increment_counter(void);


  private:
    profile_config config_;
    uint8_t counter_;
    boost::mutex protect_mutex_;
};

} // namespace profile01
} // namespace e2e
} // namespace vsomeip

#endif // VSOMEIP_E2E_PROFILE01_PROTECTOR_HPP
