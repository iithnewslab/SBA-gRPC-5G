//  Copyright (c) 2014-2017 Andrey Upadyshev <oliora@gmail.com>
//
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "ppconsul/catalog.h"
#include "s11n_types.h"


namespace json11 {
    void load(const json11::Json& src, ppconsul::catalog::NodeService& dst)
    {
        using ppconsul::s11n::load;

        load(src, dst.first);
        load(src, dst.second.id, "ServiceID");
        load(src, dst.second.name, "ServiceName");
        load(src, dst.second.address, "ServiceAddress");
        load(src, dst.second.port, "ServicePort");
        load(src, dst.second.tags, "ServiceTags");
    }

    void load(const json11::Json& src, ppconsul::catalog::NodeServices& dst)
    {
        using ppconsul::s11n::load;

        load(src, dst.first, "Node");
        load(src, dst.second, "Services");
    }
}

namespace ppconsul { namespace catalog {
    
namespace impl {

    namespace {
        s11n::Json::array to_json(const Tags& tags)
        {
            return s11n::Json::array(tags.begin(), tags.end());
        }
    }

    std::vector<std::string> parseDatacenters(const std::string& json)
    {
        return s11n::parseJson<std::vector<std::string>>(json);
    }

    std::vector<Node> parseNodes(const std::string& json)
    {
        return s11n::parseJson<std::vector<Node>>(json);
    }

    NodeServices parseNode(const std::string& json)
    {
        return s11n::parseJson<NodeServices>(json);
    }

    std::unordered_map<std::string, Tags> parseServices(const std::string& json)
    {
        return s11n::parseJson<std::unordered_map<std::string, Tags>>(json);
    }

    std::vector<NodeService> parseService(const std::string& json)
    {
        return s11n::parseJson<std::vector<NodeService>>(json);
    }

    std::string serviceRegistrationJson(const ServiceRegistrationData& service)
    {
        using s11n::Json;

        Json::object o {
            { "Node", service.node },
            { "Address", service.address },
        };

        Json::object service_o {
            { "ID", service.id },
            { "Service", service.service },
            { "Tags", to_json(service.tags) },
            { "Port", service.port },
            { "EnableTagOverride", service.enableTagOverride },
            { "Address", service.address }
        };

        o["Service"] = service_o;
        return Json(std::move(o)).dump();
    }

}
}}
