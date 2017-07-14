// EnergyPlus, Copyright (c) 1996-2017, The Board of Trustees of the University of Illinois and
// The Regents of the University of California, through Lawrence Berkeley National Laboratory
// (subject to receipt of any required approvals from the U.S. Dept. of Energy). All rights
// reserved.
//
// NOTICE: This Software was developed under funding from the U.S. Department of Energy and the
// U.S. Government consequently retains certain rights. As such, the U.S. Government has been
// granted for itself and others acting on its behalf a paid-up, nonexclusive, irrevocable,
// worldwide license in the Software to reproduce, distribute copies to the public, prepare
// derivative works, and perform publicly and display publicly, and to permit others to do so.
//
// Redistribution and use in source and binary forms, with or without modification, are permitted
// provided that the following conditions are met:
//
// (1) Redistributions of source code must retain the above copyright notice, this list of
//     conditions and the following disclaimer.
//
// (2) Redistributions in binary form must reproduce the above copyright notice, this list of
//     conditions and the following disclaimer in the documentation and/or other materials
//     provided with the distribution.
//
// (3) Neither the name of the University of California, Lawrence Berkeley National Laboratory,
//     the University of Illinois, U.S. Dept. of Energy nor the names of its contributors may be
//     used to endorse or promote products derived from this software without specific prior
//     written permission.
//
// (4) Use of EnergyPlus(TM) Name. If Licensee (i) distributes the software in stand-alone form
//     without changes from the version obtained under this License, or (ii) Licensee makes a
//     reference solely to the software portion of its product, Licensee must refer to the
//     software as "EnergyPlus version X" software, where "X" is the version number Licensee
//     obtained under this License and may not use a different name for the software. Except as
//     specifically required in this Section (4), Licensee shall not use in a company name, a
//     product name, in advertising, publicity, or other promotional activities any name, trade
//     name, trademark, logo, or other designation of "EnergyPlus", "E+", "e+" or confusingly
//     similar designation, without the U.S. Department of Energy's prior written consent.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef InputProcessor_hh_INCLUDED
#define InputProcessor_hh_INCLUDED

// C++ Headers
#include <iosfwd>
#include <type_traits>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <fstream>


// ObjexxFCL Headers
#include <ObjexxFCL/Array1S.fwd.hh>
#include <ObjexxFCL/Array1D.hh>
#include <ObjexxFCL/Optional.hh>

#include <nlohmann/json.hpp>

// EnergyPlus Headers
#include <EnergyPlus.hh>
#include <DataGlobals.hh>

using json = nlohmann::json;

namespace EnergyPlus {
class IdfParser {
public:
	friend class InputProcessorFixture;
	friend class EnergyPlusFixture;

	json decode( std::string const & idf, json const & schema );

	json decode( std::string const & idf, json const & schema, bool & success );

	std::string encode( json const & root, json const & schema );

	enum class Token : size_t {
		NONE = 0, END = 1, EXCLAMATION = 2, COMMA = 3, SEMICOLON = 4, STRING = 5, NUMBER = 6
	};

	json parse_idf( std::string const & idf, size_t & index, bool & success, json const & schema );

	json parse_object( std::string const & idf, size_t & index, bool & success, json const & schema_loc,
								json const & obj_loc );

	json parse_value( std::string const & idf, size_t & index, bool & success, json const & field_loc );

	json parse_number( std::string const & idf, size_t & index, bool & success );

	std::string parse_string( std::string const & idf, size_t & index, bool & success );

	void eat_whitespace( std::string const & idf, size_t & index );

	void eat_comment( std::string const & idf, size_t & index );

	void print_out_line_error( std::string const & idf, bool obj_found );

	void increment_both_index( size_t & index, size_t & line_index );

	void decrement_both_index( size_t & index, size_t & line_index );

	Token look_ahead( std::string const & idf, size_t index );

	Token next_token( std::string const & idf, size_t & index );

	inline std::string rtrim( std::string & s ) {
		if ( s.size() == 0 ) return std::string();
		for ( size_t i = s.size() - 1; i > 0; i-- ) {
			if ( s[ i ] != ' ' ) break;
			s.erase( i );
		}
		return s;
	}

private:
	size_t cur_line_num = 1;
	size_t index_into_cur_line = 0;
	size_t beginning_of_line_index = 0;
	char s[ 129 ];
};

class State {
public:
	enum class ErrorType {
		Maximum,
		ExclusiveMaximum,
		Minimum,
		ExclusiveMinimum
	};

	friend class InputProcessorFixture;
	friend class EnergyPlusFixture;

	void initialize( json const * parsed_schema );

	void traverse( json::parse_event_t & event, json & parsed, unsigned line_num, unsigned line_index );

	void validate( json & parsed, unsigned line_num, unsigned line_index );

	void add_error( ErrorType err, double val, unsigned line_num, unsigned line_index );

	int print_errors();

	std::vector < std::string > const & validation_errors();

	std::vector < std::string > const & validation_warnings();

private:
	json const * schema;
	std::vector < json const * > stack;
	std::unordered_map < std::string, bool > obj_required;
	std::unordered_map < std::string, bool > extensible_required;
	std::unordered_map < std::string, bool > root_required;
	// this design decision was made because
	// the choice was between sorting a vector for binary searching or log time object lookup in a map
	std::string cur_obj_name = "";

	unsigned prev_line_index = 0;
	unsigned prev_key_len = 0;
	unsigned cur_obj_count = 0;
	bool is_in_extensibles = false;
	bool does_key_exist = true;
	bool need_new_object_name = true;
	json::parse_event_t last_seen_event = json::parse_event_t::object_start;
	char s[ 129 ];
	char s2[ 129 ];

	std::vector < std::string > errors;
	std::vector < std::string > warnings;
};
}

namespace EnergyPlus {

	class InputProcessor {
	public:
		friend class EnergyPlusFixture;
		friend class InputProcessorFixture;
		static State state;
	private:
		static
		std::vector < std::string > const &
		validation_errors();

		static
		std::vector < std::string > const &
		validation_warnings();

		static IdfParser idf_parser;
		static json schema;
		static json jdf;
		//static json::parser_callback_t call_back;
		static std::unordered_map< std::string, std::string > case_insensitive_object_map;
		static std::unordered_map< std::string, std::pair< json::const_iterator, std::vector< json::const_iterator > > > jdd_jdf_cache_map;
		static std::map< const json::object_t * const, std::pair< std::string, std::string > > unused_inputs;
		static char s[ 129 ];

	public:
		static
		std::pair< bool, std::string >
		ConvertInsensitiveObjectType( std::string const & objectType );

		// Clears the global data in InputProcessor.
		// Needed for unit tests, should not be normally called.
		static
		void
		clear_state();

		static
		void
		InitFiles();

		static
		void
		InitializeMaps();

		static
		void
		ProcessInput();

		static
		int
		GetNumSectionsFound( std::string const & SectionWord );

		static
		int
		GetNumObjectsFound( std::string const & ObjectWord );

		static
		void
		GetObjectItem(
			std::string const & Object,
			int const Number,
			Array1S_string Alphas,
			int & NumAlphas,
			Array1S< Real64 > Numbers,
			int & NumNumbers,
			int & Status,
			Optional< Array1D_bool > NumBlank = _,
			Optional< Array1D_bool > AlphaBlank = _,
			Optional< Array1D_string > AlphaFieldNames = _,
			Optional< Array1D_string > NumericFieldNames = _
		);


		static
		int
		GetObjectItemNum(
			std::string const & ObjType, // Object Type (ref: IDD Objects)
			std::string const & ObjName // Name of the object type
		);

		static
		int
		GetObjectItemNum(
			std::string const & ObjType, // Object Type (ref: IDD Objects)
			std::string const & NameTypeVal, // Object "name" field type ( used as search key )
			std::string const & ObjName // Name of the object type
		);

		static
		void
		RangeCheck(
			bool & ErrorsFound, // Set to true if error detected
			std::string const & WhatFieldString, // Descriptive field for string
			std::string const & WhatObjectString, // Descriptive field for object, Zone Name, etc.
			std::string const & ErrorLevel, // 'Warning','Severe','Fatal')
			Optional_string_const LowerBoundString = _, // String for error message, if applicable
			Optional_bool_const LowerBoundCondition = _, // Condition for error condition, if applicable
			Optional_string_const UpperBoundString = _, // String for error message, if applicable
			Optional_bool_const UpperBoundCondition = _, // Condition for error condition, if applicable
			Optional_string_const ValueString = _, // Value with digits if to be displayed with error
			Optional_string_const WhatObjectName = _ // ObjectName -- used for error messages
		);

		static
		void
		GetMaxSchemaArgs(
			int & NumArgs,
			int & NumAlpha,
			int & NumNumeric
		);

		static
		void
		GetObjectDefMaxArgs(
			std::string const & ObjectWord, // Object for definition
			int & NumArgs, // How many arguments (max) this Object can have
			int & NumAlpha, // How many Alpha arguments (max) this Object can have
			int & NumNumeric // How many Numeric arguments (max) this Object can have
		);

		static
		void
		PreProcessorCheck( bool & PreP_Fatal ); // True if a preprocessor flags a fatal error

		static
		void
		PreScanReportingVariables();

		static
		void
		AddVariablesForMonthlyReport( std::string const & reportName );

		static
		void
		AddRecordToOutputVariableStructure(
			std::string const & KeyValue,
			std::string const & VariableName
		);

		static
		void
		ReAllocateAndPreserveOutputVariablesForSimulation();

		static
		void
		ReportOrphanRecordObjects();

	}; // InputProcessor
}

#endif
