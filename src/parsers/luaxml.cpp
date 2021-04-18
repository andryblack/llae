#include <pugixml/pugixml.hpp>
#include "lua/bind.h"
#include "uv/buffer.h"

namespace xml {


    static void parse_node(pugi::xml_document& doc,lua::state& l,pugi::xml_node& node) {
        l.createtable();
        l.pushstring(node.name());
        l.setfield(-2,"name");
        l.pushstring(node.value());
        l.setfield(-2,"value");
        l.pushinteger(node.type());
        l.setfield(-2,"type");

        l.createtable();
        for (pugi::xml_attribute_iterator ait = node.attributes_begin(); ait != node.attributes_end(); ++ait) {
            l.pushstring(ait->value());
            l.setfield(-2,ait->name());
        }
        l.setfield(-2,"attributes");

        l.createtable();
        lua_Integer i = 1;
        for (pugi::xml_node_iterator it = node.begin(); it != node.end(); ++it) {
            parse_node(doc,l,*it);
            l.seti(-2,i);
        }
        l.setfield(-2,"childs");
    }

    static lua::multiret tolua(lua::state& l) {
        auto buffer = uv::buffer::get(l,1);
        if (!buffer) {
            l.argerror(1,"need data");
        }
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_buffer_inplace(buffer->get_base(),buffer->get_len());
        if (!result) {
            l.pushnil();
            l.pushstring(result.description());
            return {2};
        }
        parse_node(doc,l,doc);
        return {1};
    }

}

int luaopen_xml(lua_State* L) {
	lua::state l(L);
	
	l.createtable();

	lua::bind::function(l,"tolua",&xml::tolua);

    l.createtable();
    lua::bind::value(l,"null",         pugi::node_null);
    lua::bind::value(l,"document",     pugi::node_document);
    lua::bind::value(l,"element",      pugi::node_element);
    lua::bind::value(l,"pcdata",       pugi::node_pcdata);
    lua::bind::value(l,"comment",      pugi::node_comment);
    lua::bind::value(l,"pi",           pugi::node_pi);
    lua::bind::value(l,"declaration",  pugi::node_declaration);
    lua::bind::value(l,"doctype",      pugi::node_doctype);
    l.setfield(-2,"node_type");

	return 1;
} 
