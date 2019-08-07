// Copyright (C) 2016-2017 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <sstream>
#include <vector>
#include <stdlib.h>
#include <iostream>

#include <boost/function.hpp>
#include <boost/dll/import.hpp>

#include <vsomeip/plugins/application_plugin.hpp>
#include <vsomeip/plugins/pre_configuration_plugin.hpp>

#include "../include/plugin_manager.hpp"
#include "../../configuration/include/internal.hpp"
#include "../../logging/include/logger.hpp"
#include "../../utility/include/utility.hpp"

namespace vsomeip {

std::shared_ptr<plugin_manager> plugin_manager::the_plugin_manager__ =
        std::make_shared<plugin_manager>();

std::shared_ptr<plugin_manager> plugin_manager::get() {
    return the_plugin_manager__;
}

plugin_manager::plugin_manager() :
        plugins_loaded_(false) {
}

plugin_manager::~plugin_manager() {
    handles_.clear();
    plugins_.clear();
}

void plugin_manager::load_plugins() {
    {
        boost::lock_guard<boost::mutex> its_lock_start_stop(loader_mutex_);
        if (plugins_loaded_) {
            return;
        }
        plugins_loaded_ = true;
    }

    // Get plug-ins libraries from environment
    std::vector<std::string> plugins;
    const char *its_plugins = getenv(VSOMEIP_ENV_LOAD_PLUGINS);
    if (nullptr != its_plugins) {
        std::string token;
        std::stringstream ss(its_plugins);
        while(std::getline(ss, token, ',')) {
            plugins.push_back(token);
        }
    }

    boost::lock_guard<boost::recursive_mutex> its_lock_start_stop(plugins_mutex_);
    // Load plug-in info from libraries parsed before
    for (auto plugin_name : plugins) {
		try {
			std::shared_ptr<boost::dll::shared_library> handle = std::make_shared<boost::dll::shared_library> (plugin_name, boost::dll::load_mode::rtld_lazy | boost::dll::load_mode::rtld_global);
			plugin_init_func its_init_func = handle->get_alias<plugin_init_func>(VSOMEIP_PLUGIN_INIT_SYMBOL);	
			create_plugin_func its_create_func = (*its_init_func)();
			auto its_plugin = (*its_create_func)();
			if (its_plugin) {
				handles_[its_plugin->get_plugin_type()][plugin_name] = handle;
				switch (its_plugin->get_plugin_type()) {
				case plugin_type_e::APPLICATION_PLUGIN:
					if (its_plugin->get_plugin_version()
							== VSOMEIP_APPLICATION_PLUGIN_VERSION) {
						add_plugin(its_plugin, plugin_name);
					} else {
						VSOMEIP_ERROR << "Plugin version mismatch. "
								<< "Ignoring application plugin "
								<< its_plugin->get_plugin_name();
					}
					break;
				case plugin_type_e::PRE_CONFIGURATION_PLUGIN:
					if (its_plugin->get_plugin_version()
							== VSOMEIP_PRE_CONFIGURATION_PLUGIN_VERSION) {
						add_plugin(its_plugin, plugin_name);
					} else {
						VSOMEIP_ERROR << "Plugin version mismatch. Ignoring "
								<< "pre-configuration plugin "
								<< its_plugin->get_plugin_name();
					}
					break;
				default:
					break;
				}
			}
		} catch (const boost::system::system_error& e) {
			VSOMEIP_ERROR << "Load plugin " << plugin_name << " failed: " << e.what();
			continue;
		}
    }
}

std::shared_ptr<plugin> plugin_manager::get_plugin(plugin_type_e _type, std::string _name) {
    boost::lock_guard<boost::recursive_mutex> its_lock_start_stop(plugins_mutex_);
    auto its_type = plugins_.find(_type);
    if (its_type != plugins_.end()) {
        auto its_name = its_type->second.find(_name);
        if (its_name != its_type->second.end()) {
            return its_name->second;
        }
    }
    return load_plugin(_name, _type, 1);
}

std::shared_ptr<plugin> plugin_manager::load_plugin(const std::string _library,
        plugin_type_e _type, uint32_t _version) {
	try {
		std::shared_ptr<boost::dll::shared_library> handle = std::make_shared<boost::dll::shared_library> (_library, boost::dll::load_mode::rtld_lazy | boost::dll::load_mode::rtld_global);
		plugin_init_func its_init_func = handle->get_alias<plugin_init_func>(VSOMEIP_PLUGIN_INIT_SYMBOL);	
		create_plugin_func its_create_func = (*its_init_func)();
		handles_[_type][_library] = handle;
		auto its_plugin = (*its_create_func)();
		if (its_plugin) {
			if (its_plugin->get_plugin_type() == _type
					&& its_plugin->get_plugin_version() == _version) {
				add_plugin(its_plugin, _library);
				return its_plugin;
			} else {
				VSOMEIP_ERROR << "Plugin version mismatch. Ignoring plugin "
						<< its_plugin->get_plugin_name();
			}
		}
	} catch (const boost::system::system_error& e) {
		VSOMEIP_ERROR << "Load plugin " << _library << " failed: " << e.what();
	}
	
    return nullptr;
}

bool plugin_manager::unload_plugin(plugin_type_e _type) {
    boost::lock_guard<boost::recursive_mutex> its_lock_start_stop(plugins_mutex_);
    const auto found_handle = handles_.find(_type);
    if (found_handle != handles_.end()) {
        for (auto its_name : found_handle->second) {
			its_name.second->unload();
        }
    } else {
        VSOMEIP_ERROR << "plugin_manager::unload_plugin didn't find plugin"
                << " type:" << (int)_type;
        return false;
    }
    return plugins_.erase(_type);
}

void plugin_manager::add_plugin(const std::shared_ptr<plugin> &_plugin, const std::string _name) {
    plugins_[_plugin->get_plugin_type()][_name] = _plugin;
}

}
