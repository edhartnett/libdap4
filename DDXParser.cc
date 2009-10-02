
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2003 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "config.h"

//#define DODS_DEBUG 1
//#define DODS_DEBUG2 1

#include <cstring>

#include "BaseType.h"
#include "Byte.h"
#include "Int16.h"
#include "UInt16.h"
#include "Int32.h"
#include "UInt32.h"
#include "Float32.h"
#include "Float64.h"
#include "Str.h"
#include "Url.h"
#include "Array.h"
#include "Structure.h"
#include "Sequence.h"
#include "Grid.h"

#include "DDXParser.h"

//#define DODS_DEBUG 1
//#define DODS_DEBUG2 1

#include "util.h"
#include "debug.h"
#include "mime_util.h"

namespace libdap {

static const not_used char *states[] =
    {
        "start",

        "dataset",

        "attribute_container",
        "attribute",
        "attribute_value",
        "other_xml_attribute",

        "alias",

        "simple_type",

        "array",
        "dimension",

        "grid",
        "map",

        "structure",
        "sequence",

        "blob href",

        "unknown",
        "error"
    };

// Glue the BaseTypeFactory to the enum-based factory defined statically
// here.
BaseType *DDXParser::factory(Type t, const string & name)
{
    switch (t) {
    case dods_byte_c:
        return d_factory->NewByte(name);
        break;

    case dods_int16_c:
        return d_factory->NewInt16(name);
        break;

    case dods_uint16_c:
        return d_factory->NewUInt16(name);
        break;

    case dods_int32_c:
        return d_factory->NewInt32(name);
        break;

    case dods_uint32_c:
        return d_factory->NewUInt32(name);
        break;

    case dods_float32_c:
        return d_factory->NewFloat32(name);
        break;

    case dods_float64_c:
        return d_factory->NewFloat64(name);
        break;

    case dods_str_c:
        return d_factory->NewStr(name);
        break;

    case dods_url_c:
        return d_factory->NewUrl(name);
        break;

    case dods_array_c:
        return d_factory->NewArray(name);
        break;

    case dods_structure_c:
        return d_factory->NewStructure(name);
        break;

    case dods_sequence_c:
        return d_factory->NewSequence(name);
        break;

    case dods_grid_c:
        return d_factory->NewGrid(name);
        break;

    default:
        return 0;
    }
}

/** Get the Type enumeration value which matches the given name. */
static Type get_type(const char *name)
{
    if (strcmp(name, "Byte") == 0)
        return dods_byte_c;

    if (strcmp(name, "Int16") == 0)
        return dods_int16_c;

    if (strcmp(name, "UInt16") == 0)
        return dods_uint16_c;

    if (strcmp(name, "Int32") == 0)
        return dods_int32_c;

    if (strcmp(name, "UInt32") == 0)
        return dods_uint32_c;

    if (strcmp(name, "Float32") == 0)
        return dods_float32_c;

    if (strcmp(name, "Float64") == 0)
        return dods_float64_c;

    if (strcmp(name, "String") == 0)
        return dods_str_c;

    if (strcmp(name, "Url") == 0)
        return dods_url_c;

    if (strcmp(name, "Array") == 0)
        return dods_array_c;

    if (strcmp(name, "Structure") == 0)
        return dods_structure_c;

    if (strcmp(name, "Sequence") == 0)
        return dods_sequence_c;

    if (strcmp(name, "Grid") == 0)
        return dods_grid_c;

    return dods_null_c;
}

static Type is_simple_type(const char *name)
{
    Type t = get_type(name);
    switch (t) {
    case dods_byte_c:
    case dods_int16_c:
    case dods_uint16_c:
    case dods_int32_c:
    case dods_uint32_c:
    case dods_float32_c:
    case dods_float64_c:
    case dods_str_c:
    case dods_url_c:
        return t;
    default:
        return dods_null_c;
    }
}

static bool is_not(const char *name, const char *tag)
{
    return strcmp(name, tag) != 0;
}

void DDXParser::set_state(DDXParser::ParseState state)
{
    s.push(state);
}

DDXParser::ParseState DDXParser::get_state() const
{
    return s.top();
}

void DDXParser::pop_state()
{
    s.pop();
}

/** Dump XML attributes to local store so they can be easily manipulated.
    Attribute names are always folded to lower case.
    @param attrs The XML attribute array */
void DDXParser::transfer_attrs(const char **attrs)
{
    attributes.clear();         // erase old attributes

    if (!attrs)
        return;

    for (int i = 0; attrs[i] != 0; i += 2) {
        DBG(cerr << "attrs[" << i << "]: " << attrs[i]);
        DBG(cerr << "attrs[" << i << "+1]: " << attrs[i+1]);
        string attr_i = attrs[i];
        downcase(attr_i);
        attributes[attr_i] = string(attrs[i + 1]);
    }
}

/** Is an attribute present? Attribute names are always lower case.
    @note To use this method, first call transfer_attrs.
    @param attr The XML attribute
    @return True if the XML attribute was present in the last tag */
bool DDXParser::check_required_attribute(const string & attr)
{
    map < string, string >::iterator i = attributes.find(attr);
    if (i == attributes.end())
        ddx_fatal_error(this, "Required attribute '%s' not found.",
                        attr.c_str());
    return true;
}

/** Is an attribute present? Attribute names are always lower case.
    @note To use this method, first call transfer_attrs.
    @param attr The XML attribute
    @return True if the XML attribute was present in the last/current tag,
    false otherwise. */
bool DDXParser::check_attribute(const string & attr)
{
    return (attributes.find(attr) != attributes.end());
}

/** Given that an \c Attribute tag has just been read, determine whether the
    element is a container or a simple type, set the state and, for a simple
    type record the type and name for use when \c value elements are found.

    @note Modified to discriminate between OtherXML and the older DAP2.0
    attribute types (container, Byte, ...).

    @param attrs The array of XML attribute values */
void DDXParser::process_attribute_element(const char **attrs)
{
    // These methods set the state to parser_error if a problem is found.
    transfer_attrs(attrs);
    bool error = !(check_required_attribute(string("name"))
                   && check_required_attribute(string("type")));
    if (error)
        return;

    if (attributes["type"] == "Container") {
        set_state(inside_attribute_container);

        AttrTable *child;
        AttrTable *parent = at_stack.top();

        child = parent->append_container(attributes["name"]);
        at_stack.push(child);   // save.
        DBG2(cerr << "Pushing at" << endl);
    }
    else if (attributes["type"] == "OtherXML") {
        set_state(inside_other_xml_attribute);

        dods_attr_name = attributes["name"];
        dods_attr_type = attributes["type"];
    }
    else {
        set_state(inside_attribute);
        // *** Modify parser. Add a special state for inside OtherXML since it
        // does not use the <value> element.

        dods_attr_name = attributes["name"];
        dods_attr_type = attributes["type"];
    }
}

/** Given that an \c Alias tag has just been read, set the state and process
    the alias.
    @param attrs The XML attribute array */
void DDXParser::process_attribute_alias(const char **attrs)
{
    transfer_attrs(attrs);
    if (check_required_attribute(string("name"))
        && check_required_attribute(string("attribute"))) {
        set_state(inside_alias);
        at_stack.top()->attr_alias(attributes["name"],
                                   attributes["attribute"]);
    }
}

/** Given that a tag which opens a variable declaration has just been read,
    create the variable. Once created, push the variable onto the stack of
    variables, push that variables attribute table onto the attribute table
    stack and update the state of the parser.
    @param t The type of variable to create.
    @param s The next state of the parser.
    @param attrs the attributes read with the tag */
void DDXParser::process_variable(Type t, ParseState s, const char **attrs)
{
    transfer_attrs(attrs);

    set_state(s);
    if (bt_stack.top()->type() == dods_array_c
            || check_required_attribute("name")) { // throws on error/false
        BaseType *btp = factory(t, attributes["name"]);
        if (!btp)
            ddx_fatal_error(
                    this,
                    "Internal parser error; could not instantiate the variable '%s'.",
                    attributes["name"].c_str());

        // Once we make the new variable, we not only load it on to the
        // BaseType stack, we also load its AttrTable on the AttrTable stack.
        // The attribute processing software always operates on the AttrTable
        // at the top of the AttrTable stack (at_stack).
        bt_stack.push(btp);
        at_stack.push(&btp->get_attr_table());
    }
}

/** Given that a \c dimension tag has just been read, add that information to
    the array on the top of the BaseType stack.
    @param attrs The XML attributes included in the \c dimension tag */
void DDXParser::process_dimension(const char **attrs)
{
    transfer_attrs(attrs);
    if (check_required_attribute(string("size"))) {
        set_state(inside_dimension);
        Array *ap = dynamic_cast<Array *> (bt_stack.top());
        if (!ap)
            ddx_fatal_error(this, "Parse error: Expected an array variable.");
        else
            ap->append_dim(atoi(attributes["size"].c_str()),
                    attributes["name"]);
    }
}

/** Given that a \c blob tag has just been read, extract and save the URL
    included in the element.

        @param attrs The XML attributes */
void DDXParser::process_blob(const char **attrs)
{
#if 0
    if (dds->get_dap_major() > 2 && dds->get_dap_major() > 1)
        ddx_fatal_error(this,
                "Found a blob element in a 3.2 or greater response.");
#endif
    transfer_attrs(attrs);
    if (check_required_attribute(string("href"))) {
        set_state(inside_blob_href);
        *blob_href = attributes["href"];
    }
}

/** Check to see if the current tag is either an \c Attribute or an \c Alias
    start tag. This method is a glorified macro...

    @param name The start tag name
    @param attrs The tag's XML attributes
    @return True if the tag was an \c Attribute or \c Alias tag */
inline bool
DDXParser::is_attribute_or_alias(const char *name, const char **attrs)
{
    if (strcmp(name, "Attribute") == 0) {
        process_attribute_element(attrs);
        // next state: inside_attribtue or inside_attribute_container
        return true;
    }
    else if (strcmp(name, "Alias") == 0) {
        process_attribute_alias(attrs);
        // next state: inside_alias
        return true;
    }

    return false;
}

/** Check to see if the current tag is the start of a variable declaration.
    If so, process it. A glorified macro...
    @param name The start tag name
    @param attrs The tag's XML attributes
    @return True if the tag was a variable tag */
inline bool DDXParser::is_variable(const char *name, const char **attrs)
{
    Type t;
    if ((t = is_simple_type(name)) != dods_null_c) {
        process_variable(t, inside_simple_type, attrs);
        return true;
    }
    else if (strcmp(name, "Array") == 0) {
        process_variable(dods_array_c, inside_array, attrs);
        return true;
    }
    else if (strcmp(name, "Structure") == 0) {
        process_variable(dods_structure_c, inside_structure, attrs);
        return true;
    }
    else if (strcmp(name, "Sequence") == 0) {
        process_variable(dods_sequence_c, inside_sequence, attrs);
        return true;
    }
    else if (strcmp(name, "Grid") == 0) {
        process_variable(dods_grid_c, inside_grid, attrs);
        return true;
    }

    return false;
}

void DDXParser::finish_variable(const char *tag, Type t,
                                const char *expected)
{
    if (strcmp(tag, expected) != 0) {
        DDXParser::ddx_fatal_error(this,
                                   "Expected an end tag for a %s; found '%s' instead.",
                                   expected, tag);
        return;
    }

    pop_state();

    BaseType *btp = bt_stack.top();

    bt_stack.pop();
    at_stack.pop();

    if (btp && btp->type() != t) {
        DDXParser::ddx_fatal_error(this,
                                   "Internal error: Expected a %s variable.",
                                   expected);
        return;
    }
    // Once libxml2 validates, this can go away. 05/30/03 jhrg
    if (t == dods_array_c
        && dynamic_cast < Array * >(btp)->dimensions() == 0) {
        DDXParser::ddx_fatal_error(this,
                                   "No dimension element included in the Array '%s'.",
                                   btp->name().c_str());
        return;
    }

    BaseType *parent = bt_stack.top();

    if (!(parent->is_vector_type() || parent->is_constructor_type())) {
        DDXParser::ddx_fatal_error(this,
                                   "Tried to add the array variable '%s' to a non-constructor type (%s %s).",
                                   tag,
                                   bt_stack.top()->type_name().c_str(),
                                   bt_stack.top()->name().c_str());
        return;
    }

    parent->add_var(btp);
}

/** @name SAX Parser Callbacks

    These methods are declared static in the class header. This gives them C
    linkage which allows them to be used as callbacks by the SAX parser
    engine. */
//@{

/** Initialize the SAX parser state object. This object is passed to each
    callback as a void pointer. The initial state is parser_start.

    @param parser The SAX parser  */
void DDXParser::ddx_start_document(DDXParser * parser)
{
    parser->error_msg = "";
    parser->char_data = "";

    // init attr table stack.
    parser->at_stack.push(&parser->dds->get_attr_table());

    // Trick; DDS *should* be a child of Structure. To simplify parsing,
    // stuff a Structure on the bt_stack and dump the top level variables
    // there. Once we're done, transfer the variables to the DDS.
    parser->bt_stack.push(new Structure("dummy_dds"));

    parser->set_state(parser_start);

    DBG2(cerr << "Parser state: " << states[parser->get_state()] << endl);
}

/** Clean up after finishing a parse.
    @param parser The SAX parser  */
void DDXParser::ddx_end_document(DDXParser * parser)
{
    DBG2(cerr << "Ending state == " << states[parser->get_state()] <<
         endl);

    // If we've found any sort of error, don't make the DDX; intern() will
    // take care of the error.
    if (parser->get_state() == parser_error)
        return;

    if (parser->get_state() != parser_start)
        DDXParser::ddx_fatal_error(parser,
                                   "The document contained unbalanced tags.");

    // Pop the temporary Structure off the stack and transfer its variables
    // to the DDS.
    Constructor *cp = dynamic_cast < Constructor * >(parser->bt_stack.top());
    if (!cp)
        ddx_fatal_error(parser,
                "Parse error: Expected a Structure, Sequence or Grid variable.");
    else {
        for (Constructor::Vars_iter i = cp->var_begin(); i != cp->var_end(); ++i)
            parser->dds->add_var(*i);

        parser->bt_stack.pop();
        delete cp;
    }
}


/** Process a start element tag. Because the DDX schema uses attributes and
    because libxml2 does not validate those, we do attribute validation here.

    @param parser The SAX parser
    @param name The element
    @param attrs character array of attribute names. */
void DDXParser::ddx_start_element(DDXParser * parser, const char *name,
                                  const char **attrs)
{
    DBG2(cerr << "start element: " << name << ", states: "
         << states[parser->get_state()]);

    switch (parser->get_state()) {
    case parser_start:
        if (strcmp(name, "Dataset") == 0) {
            parser->set_state(inside_dataset);

            parser->transfer_attrs(attrs);

            if (parser->check_required_attribute(string("name")))
                parser->dds->set_dataset_name(parser->attributes["name"]);

            if (parser->check_attribute("dap_version"))
                parser->dds->set_dap_version(parser->attributes["dap_version"]);
        }
        else
            DDXParser::ddx_fatal_error(parser,
                                       "Expected response to start with a Dataset element; found '%s' instead.",
                                       name);
        break;

    case inside_dataset:
        if (parser->is_attribute_or_alias(name, attrs))
            break;
        else if (parser->is_variable(name, attrs))
            break;
        else if (strcmp(name, "blob") == 0 || strcmp(name, "dataBLOB") == 0) {
            parser->process_blob(attrs);
            // next state: inside_blob_href
        }
        else
            DDXParser::ddx_fatal_error(parser,
                                       "Expected an Attribute, Alias or variable element; found '%s' instead.",
                                       name);
        break;

    case inside_attribute_container:
        if (parser->is_attribute_or_alias(name, attrs))
            break;
        else
            DDXParser::ddx_fatal_error(parser,
                                       "Expected an Attribute or Alias element; found '%s' instead.",
                                       name);
        break;

    case inside_attribute:
        if (parser->is_attribute_or_alias(name, attrs))
            break;
        else if (strcmp(name, "value") == 0)
            parser->set_state(inside_attribute_value);
        else
            ddx_fatal_error(parser,
                            "Expected an 'Attribute', 'Alias' or 'value' element; found '%s' instead.",
                            name);
        break;

    case inside_attribute_value:
        ddx_fatal_error(parser,
                        "Internal parser error; unexpected state, inside value while processing element '%s'.",
                        name);
        break;

    case inside_other_xml_attribute:
        DBGN(cerr << endl << "\t inside_other_xml_attribute: " << name << endl);

        parser->other_xml_depth++;

        // Accumulate the elements here
        parser->other_xml.append("<");
        parser->other_xml.append(name);

        if (!attrs) {
            parser->other_xml.append(">");
            break;
        }

        for (int i = 0; attrs[i] != 0; i += 2) {
            parser->other_xml.append(" ");
            parser->other_xml.append(attrs[i]);
            parser->other_xml.append("=\"");
            parser->other_xml.append(attrs[i+1]);
            parser->other_xml.append("\"");
        }

        parser->other_xml.append(">");
        break;

    case inside_alias:
        ddx_fatal_error(parser,
                        "Internal parser error; unexpected state, inside alias while processing element '%s'.",
                        name);
        break;

    case inside_simple_type:
        if (parser->is_attribute_or_alias(name, attrs))
            break;
        else
            ddx_fatal_error(parser,
                            "Expected an 'Attribute' or 'Alias' element; found '%s' instead.",
                            name);
        break;

    case inside_array:
        if (parser->is_attribute_or_alias(name, attrs))
            break;
        else if (is_not(name, "Array") && parser->is_variable(name, attrs))
            break;
        else if (strcmp(name, "dimension") == 0) {
            parser->process_dimension(attrs);
            // next state: inside_dimension
        }
        else
            ddx_fatal_error(parser,
                            "Expected an 'Attribute' or 'Alias' element; found '%s' instead.",
                            name);
        break;

    case inside_dimension:
        ddx_fatal_error(parser,
                        "Internal parser error; unexpected state, inside dimension while processing element '%s'.",
                        name);
        break;

    case inside_structure:
        if (parser->is_attribute_or_alias(name, attrs))
            break;
        else if (parser->is_variable(name, attrs))
            break;
        else
            DDXParser::ddx_fatal_error(parser,
                                       "Expected an Attribute, Alias or variable element; found '%s' instead.",
                                       name);
        break;

    case inside_sequence:
        if (parser->is_attribute_or_alias(name, attrs))
            break;
        else if (parser->is_variable(name, attrs))
            break;
        else
            DDXParser::ddx_fatal_error(parser,
                                       "Expected an Attribute, Alias or variable element; found '%s' instead.",
                                       name);
        break;

    case inside_grid:
        if (parser->is_attribute_or_alias(name, attrs))
            break;
        else if (strcmp(name, "Array") == 0)
            parser->process_variable(dods_array_c, inside_array, attrs);
        else if (strcmp(name, "Map") == 0)
            parser->process_variable(dods_array_c, inside_map, attrs);
        else
            DDXParser::ddx_fatal_error(parser,
                                       "Expected an Attribute, Alias or variable element; found '%s' instead.",
                                       name);
        break;

    case inside_map:
        if (parser->is_attribute_or_alias(name, attrs))
            break;
        else if (is_not(name, "Array") && is_not(name, "Sequence")
                 && is_not(name, "Grid")
                 && parser->is_variable(name, attrs))
            break;
        else if (strcmp(name, "dimension") == 0) {
            parser->process_dimension(attrs);
            // next state: inside_dimension
        }
        else
            ddx_fatal_error(parser,
                            "Expected an 'Attribute', 'Alias', variable or 'dimension' element; found '%s' instead.",
                            name);
        break;

    case inside_blob_href:
        ddx_fatal_error(parser,
                        "Internal parser error; unexpected state, inside blob href while processing element '%s'.",
                        name);
        break;

    case parser_unknown:
        // *** Never used? If so remove/error
        parser->set_state(parser_unknown);
        break;

    case parser_error:
        break;
    }

    DBGN(cerr << " ... " << states[parser->get_state()] << endl);
}

/** Process an end element tag. This is where values are added to the
    DDS/DDX or their parent BaseType.

    @param parser The SAX parser state
    @param name The element name. */
void DDXParser::ddx_end_element(DDXParser * parser, const char *name)
{
    DBG2(cerr << "End element " << name << " (state "
         << states[parser->get_state()] << ")" << endl);

    switch (parser->get_state()) {
    case parser_start:
        ddx_fatal_error(parser,
                        "Internal parser error; unexpected state, inside start state while processing element '%s'.",
                        name);
        break;

    case inside_dataset:
        if (strcmp(name, "Dataset") == 0) {
            parser->pop_state();
            // If the state now on the top of the stack is 'parser_start', the
            // parse is done. How to signal that?
        }
        else
            DDXParser::ddx_fatal_error(parser,
                                       "Expected an end Dataset tag; found '%s' instead.",
                                       name);
        break;

    case inside_attribute_container:
        if (strcmp(name, "Attribute") == 0) {
            parser->pop_state();
            parser->at_stack.pop();     // pop when leaving a container.
        }
        else
            DDXParser::ddx_fatal_error(parser,
                                       "Expected an end Attribute tag; found '%s' instead.",
                                       name);
        break;

    case inside_attribute:
        if (strcmp(name, "Attribute") == 0)
            parser->pop_state();
        else
            DDXParser::ddx_fatal_error(parser,
                                       "Expected an end Attribute tag; found '%s' instead.",
                                       name);
        break;

    case inside_attribute_value:
        if (strcmp(name, "value") == 0) {
            parser->pop_state();
            AttrTable *atp = parser->at_stack.top();
            atp->append_attr(parser->dods_attr_name,
                             parser->dods_attr_type, parser->char_data);
            parser->char_data = "";     // Null this after use.
        }
        else
            DDXParser::ddx_fatal_error(parser,
                                       "Expected an end value tag; found '%s' instead.",
                                       name);

        break;

    case inside_other_xml_attribute:
        if (strcmp(name, "Attribute") == 0) {
            DBGN(cerr << endl << "\t Popping the 'inside_other_xml_attribute' state"
            << endl);

            parser->pop_state();

            AttrTable *atp = parser->at_stack.top();
            atp->append_attr(parser->dods_attr_name,
                             parser->dods_attr_type, parser->other_xml);

            parser->other_xml = "";     // Null this after use.
        }
        else {
            DBGN(cerr << endl << "\t inside_other_xml_attribute: " << name
                    << ", depth: " << parser->other_xml_depth << endl);
            parser->other_xml_depth--;
            parser->other_xml.append("</");
            parser->other_xml.append(name);
            parser->other_xml.append(">");
        }
        break;

        // Alias is busted in C++ 05/29/03 jhrg
    case inside_alias:
        parser->pop_state();
        break;

    case inside_simple_type:
        if (is_simple_type(name) != dods_null_c) {
            parser->pop_state();
            BaseType *btp = parser->bt_stack.top();
            parser->bt_stack.pop();
            parser->at_stack.pop();

            BaseType *parent = parser->bt_stack.top();

            if (parent->is_vector_type() || parent->is_constructor_type())
                parent->add_var(btp);
            else
                DDXParser::ddx_fatal_error(parser,
                                           "Tried to add the simple-type variable '%s' to a non-constructor type (%s %s).",
                                           name,
                                           parser->bt_stack.top()->
                                           type_name().c_str(),
                                           parser->bt_stack.top()->name().
                                           c_str());
        }
        else
            DDXParser::ddx_fatal_error(parser,
                                       "Expected an end tag for a simple type; found '%s' instead.",
                                       name);
        break;

    case inside_array:
        parser->finish_variable(name, dods_array_c, "Array");
        break;

    case inside_dimension:
        if (strcmp(name, "dimension") == 0)
            parser->pop_state();
        else
            DDXParser::ddx_fatal_error(parser,
                                       "Expected an end dimension tag; found '%s' instead.",
                                       name);
        break;

    case inside_structure:
        parser->finish_variable(name, dods_structure_c, "Structure");
        break;

    case inside_sequence:
        parser->finish_variable(name, dods_sequence_c, "Sequence");
        break;

    case inside_grid:
        parser->finish_variable(name, dods_grid_c, "Grid");
        break;

    case inside_map:
        parser->finish_variable(name, dods_array_c, "Map");
        break;

    case inside_blob_href:
        if (strcmp(name, "blob") == 0 || strcmp(name, "dataBLOB") == 0)
            parser->pop_state();
        else
            DDXParser::ddx_fatal_error(parser,
                                       "Expected an end blob tag; found '%s' instead.",
                                       name);
        break;

    case parser_unknown:
        parser->pop_state();
        break;

    case parser_error:
        break;
    }

    DBGN(cerr << " ... " << states[parser->get_state()] << endl);
}

/** Process/accumulate character data. This may be called more than once for
    one logical clump of data. Only save character data when processing
    'value' elements; throw away all other characters. */
void DDXParser::ddx_get_characters(DDXParser * parser, const xmlChar * ch, int len)
{
    switch (parser->get_state()) {
        case inside_attribute_value:
            parser->char_data.append((const char *)(ch), len);
            DBG2(cerr << "Characters: '" << parser->char_data << "'" << endl);
            break;

        case inside_other_xml_attribute:
            parser->other_xml.append((const char *)(ch), len);
            DBG2(cerr << "Other XML Characters: '" << parser->other_xml << "'" << endl);
            break;

        default:
            break;
    }
}

/** Read whitespace that's not really important for content. This is used
    only for the OtherXML attribute type to preserve formating of the XML.
    Doing so makes the attribute value far easier to read.
 */
void DDXParser::ddx_ignoreable_whitespace(DDXParser *parser, const xmlChar *ch,
        int len)
{
    switch (parser->get_state()) {
         case inside_other_xml_attribute:
             parser->other_xml.append((const char *)(ch), len);
             break;

         default:
             break;
    }
}

/** Get characters in a cdata block. DAP does not use CData, but XML in an
    OtherXML attribute (the value of that DAP attribute) might use it. This
    callback also allows CData when the parser is in the 'parser_unknown'
    state since some future DAP element might use it.
 */
void DDXParser::ddx_get_cdata(DDXParser *parser, const xmlChar *value, int len)
{
    switch (parser->get_state()) {
         case inside_other_xml_attribute:
             parser->other_xml.append((const char *)(value), len);
             break;

         case parser_unknown:
             break;

         default:
             DDXParser::ddx_fatal_error(parser,
                                        "Found a CData block but none are allowed by DAP.");

             break;
    }
}

/** Handle the standard XML entities.

    @param parser The SAX parser
    @param name The XML entity. */
xmlEntityPtr DDXParser::ddx_get_entity(DDXParser *, const xmlChar * name)
{
    return xmlGetPredefinedEntity(name);
}

/** Process an XML fatal error. Note that SAX provides for warnings, errors
    and fatal errors. This code treats them all as fatal errors since there's
    typically no way to tell a user about the error since there's often no
    user interface for this software.

    @param parser The SAX parser
    @param msg A printf-style format string. */
void DDXParser::ddx_fatal_error(DDXParser * parser, const char *msg, ...)
{
    va_list args;

    parser->set_state(parser_error);

    va_start(args, msg);
    char str[1024];
    vsnprintf(str, 1024, msg, args);
    va_end(args);

    int line = xmlSAX2GetLineNumber(parser->ctxt);

    parser->error_msg += "At line " + long_to_string(line) + ": ";
    parser->error_msg += string(str) + string("\n");

    DBG2(cerr << "Parser Error: " << parser->error_msg << endl);
}

//@}

/** This local variable holds pointers to the callback <i>functions</i> which
    comprise the SAX parser. */
static xmlSAXHandler ddx_sax_parser =
    {
        0,                          // internalSubset
        0,                          // isStandalone
        0,                          // hasInternalSubset
        0,                          // hasExternalSubset
        0,                          // resolveEntity
        (getEntitySAXFunc) DDXParser::ddx_get_entity,       // getEntity
        0,                          // entityDecl
        0,                          // notationDecl
        0,                          // attributeDecl
        0,                          // elementDecl
        0,                          // unparsedEntityDecl
        0,                          // setDocumentLocator
        (startDocumentSAXFunc) DDXParser::ddx_start_document,       // startDocument
        (endDocumentSAXFunc) DDXParser::ddx_end_document,   // endDocument
        (startElementSAXFunc) DDXParser::ddx_start_element, // startElement
        (endElementSAXFunc) DDXParser::ddx_end_element,     // endElement
        0,                          // reference
        (charactersSAXFunc) DDXParser::ddx_get_characters,
        (ignorableWhitespaceSAXFunc) DDXParser::ddx_ignoreable_whitespace, // ignorableWhitespace
        0,                          // processingInstruction
        0,                          // (commentSAXFunc)gladeComment,  comment
        (warningSAXFunc) DDXParser::ddx_fatal_error,        // warning
        (errorSAXFunc) DDXParser::ddx_fatal_error,  // error
        (fatalErrorSAXFunc) DDXParser::ddx_fatal_error,     // fatalError
        0,                          // getParameterEntity
        0,                          // cdataBlock
        0,                          // externalSubset
        0,                          // initialized
        0,                          // _private
        (startElementNsSAX2Func) 0,  // startElementNs
        (endElementNsSAX2Func) 0,    // endElementNs
        (xmlStructuredErrorFunc)0   // serror
    };

void DDXParser::cleanup_parse(xmlParserCtxtPtr & context) const
{
    if (!context->wellFormed) {
        context->sax = NULL;
        xmlFreeParserCtxt(context);
        throw
        DDXParseFailed(string
                       ("\nThe DDX is not a well formed XML document.\n")
                       + error_msg);
    }

    if (!context->valid) {
        context->sax = NULL;
        xmlFreeParserCtxt(context);
        throw DDXParseFailed(string("\nThe DDX is not a valid document.\n")
                             + error_msg);
    }

    if (get_state() == parser_error) {
        context->sax = NULL;
        xmlFreeParserCtxt(context);
        throw DDXParseFailed(string("\nError parsing DDX response.\n") +
                             error_msg);
    }

    context->sax = NULL;
    xmlFreeParserCtxt(context);
}

/** @brief Read the DDX from a stream instead of a file.
    @param in REad from this FILE* stream
    @param boundary Stop reading/parsing when this Multipart MIME boundary is
    found
    @param dest_dds Build the DDS here
    @param blob return the data blob CID in this value-result parameter.
    @see DDXParser::intern(). */
void DDXParser::intern(FILE * in, DDS * dest_dds, string &blob,
	const string &boundary)
{
    // Code example from libxml2 docs re: read from a stream.

    if (!in || feof(in) || ferror(in))
	throw InternalErr(__FILE__, __LINE__,
		"Input stream not open or read error");

    dds = dest_dds; // dump values here
    blob_href = &blob; // blob goes here

    size_t size = 0;
    char line[1024];
    xmlParserCtxtPtr context = xmlCreatePushParserCtxt(&ddx_sax_parser, this,
	    line, size, "stream");

    ctxt = context; // need ctxt for error messages
    context->validate = true;

#if 0
    char *line = 0;
    while (((line = fgetln(in, &size)) > 0) && !is_boundary(line, boundary)) {
	DBG(cerr << "line: " << line << endl);
	xmlParseChunk(ctxt, line, size, 0);
    }
#endif

    context->sax = &ddx_sax_parser;
    context->userData = this;
    context->validate = true;

    while ((fgets(line, 1024, in) > 0) && !is_boundary(line, boundary)) {
	DBG(cerr << "line: " << line << endl);
	xmlParseChunk(ctxt, line, strlen(line), 0);
    }
    // This call ends the parse: The fourth argument of xmlParseChunk is
    // the bool 'terminate.'
    xmlParseChunk(ctxt, line, 0, 1);

    cleanup_parse(context);
}


/** Parse a DDX document stored in a file. The XML in the document is parsed
    and a binary DDX is built. This implementation stores the result in a DDS
    object where each instance of BaseType can hold an AttrTable object.

    @param document Read the DDX from this file. Assume that the document
    contains _only_ a DDX.
    @param dest_dds Value/result parameter; dumps the information to this DDS
    instance.
    @param blob Value/result parameter; puts the href which references the \c
    'blob' document here. If this is a DAP 3.2+ document then this will be
    the Content-Id of the multipart MIME part that holds the data.
    @exception DDXParseFailed Thrown if the XML document could not be
    read or parsed. */
void DDXParser::intern(const string & document, DDS * dest_dds, string &blob)
{
    // Create the context pointer explicitly so that we can store a pointer
    // to it in the DDXParser instance. This provides a way to generate our
    // own error messages *with* line numbers. The messages are pretty
    // meaningless otherwise. This means that we use an interface from the
    // 'parser internals' header, and not the 'parser' header. However, this
    // interface is also used in one of the documented examples, so it's
    // probably pretty stable. 06/02/03 jhrg
    xmlParserCtxtPtr context = xmlCreateFileParserCtxt(document.c_str());
    if (!context)
        throw
        DDXParseFailed(string
                       ("Could not initialize the parser with the file: '")
                       + document + string("'."));

    dds = dest_dds;             // dump values here
    blob_href = &blob;           // blob goes here

    ctxt = context;             // need ctxt for error messages

    context->sax = &ddx_sax_parser;
    context->userData = this;
    context->validate = true;

    xmlParseDocument(context);

    cleanup_parse(context);
}

} // namespace libdap
