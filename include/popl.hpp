/***
     ____   __  ____  __   
    (  _ \ /  \(  _ \(  )  
     ) __/(  O )) __// (_/\
    (__)   \__/(__)  \____/
    version 1.1.0
    https://github.com/badaix/popl

	This file is part of popl (program options parser lib)
    Copyright (C) 2015-2018 Johannes Pohl
    
    This software may be modified and distributed under the terms
    of the MIT license.  See the LICENSE file for details.
***/


#ifndef POPL_HPP
#define POPL_HPP

#define NOMINMAX

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>


namespace popl
{

#define POPL_VERSION "1.1.0"


enum class Argument // permitted values for its `argument_type' field...
{
	no = 0,    // option never takes an argument
	required,  // option always requires an argument
	optional   // option may take an argument
};


enum class Type // permitted values for its `argument_type' field...
{
	kValue = 0,  // option never takes an argument
	kImplicit,   // option always requires an argument
	kSwitch      // option may take an argument
};


enum class Visibility
{
	inactive = 0,
	hidden = 1,
	normal = 2,
	advanced = 3,
	expert = 4
};


/// Abstract Base class for Options
/**
 * Base class for Options
 * holds just configuration data, no runtime data
 */
class Option
{
friend class OptionParser;
public:
	Option(const std::string& short_option, const std::string& long_option, std::string description);
	virtual ~Option() = default;

	char short_option() const;
	std::string long_option() const;
	std::string description() const;

	void set_visibility(const Visibility& visibility);
	Visibility visibility() const;

	virtual Argument argument_type() const = 0;

protected:
	virtual void parse(const std::string& what_option, const char* value) = 0;
	virtual void get_help_info(Type& type, bool& has_default, std::string& default_value) const = 0;

	std::string short_option_;
	std::string long_option_;
	std::string description_;
	Visibility visibility_;
};




/// Abstract Base class for Options with runtime data
/**
 * Base class for Options with runtime data
 * Stores a value of type T with getter and setter
 */
template<class T>
class ValueTemplate : public Option
{
public:
	ValueTemplate(const std::string& short_option, const std::string& long_option, const std::string& description, T* assign_to = nullptr);

	unsigned int count() const;
	bool is_set() const;

	void assign_to(T* var);

	void set_value(const T& value);
	virtual T value(size_t idx = 0) const;

protected:
	virtual void update_reference();
	virtual void add_value(const T& value);

	unsigned int count_;
	T* assign_to_;
	std::vector<T> values_;
};




/// Value option with optional default value
/**
 * Value option with optional default value
 * If set, it requires an argument
 */
template<class T>
class Value : public ValueTemplate<T>
{
public:
	Value(const std::string& short_option, const std::string& long_option, const std::string& description);
	Value(const std::string& short_option, const std::string& long_option, const std::string& description, const T& default_val, T* assign_to = nullptr);

	void set_default(const T& value);
	bool has_default() const;
	T get_default() const;

	T value(size_t idx = 0) const override;
	Argument argument_type() const override;

protected:
	void parse(const std::string& what_option, const char* value) override;
	void update_reference() override;
	void get_help_info(Type& type, bool& has_default, std::string& default_value) const override;
	std::unique_ptr<T> default_;
};




/// Value option with implicit default value
/**
 * Value option with implicit default value
 * If set, an argument is optional
 * without argument it carries the implicit default value
 * with argument it carries the explicit value
 */
template<class T>
class Implicit : public Value<T>
{
public:
	Implicit(const std::string& short_option, const std::string& long_option, const std::string& description, const T& implicit_val, T* assign_to = nullptr);

	Argument argument_type() const override;

protected:
	void parse(const std::string& what_option, const char* value) override;
	void get_help_info(Type& type, bool& has_default, std::string& default_value) const override;
};




/// Value option without value
/**
 * Value option without value
 * Does not require an argument
 * Can be either set or not set
 */
class Switch : public ValueTemplate<bool>
{
public:
	Switch(const std::string& short_option, const std::string& long_option, const std::string& description, bool* assign_to = nullptr);

	Argument argument_type() const override;

protected:
	void parse(const std::string& what_option, const char* value) override;
	void get_help_info(Type& type, bool& has_default, std::string& default_value) const override;
};




using Option_ptr = std::shared_ptr<Option>;

/// OptionParser manages all Options
/**
 * OptionParser manages all Options
 * Add Options (Option_Type = Value<T>, Implicit<T> or Switch) with "add<Option_Type>(option params)""
 * Call "parse(argc, argv)" to trigger parsing of the options and to 
 * fill "non_option_args" and "unknown_options"
 */
class OptionParser
{
friend class HelpPrinter;
public:
	explicit OptionParser(std::string description = "");
	virtual ~OptionParser() = default;

	template<typename T, Visibility visibility, typename... Ts>
	std::shared_ptr<T> add(Ts&&... params);
	template<typename T, typename... Ts>
	std::shared_ptr<T> add(Ts&&... params);

	void parse(int argc, const char * const * argv);
	std::string help(const Visibility& max_visibility = Visibility::normal) const;
	std::string description() const;
	const std::vector<Option_ptr>& options() const;
	const std::vector<std::string>& non_option_args() const;
	const std::vector<std::string>& unknown_options() const;

	template<typename T>
	std::shared_ptr<T> get_option(const std::string& long_opt) const;
	template<typename T>
	std::shared_ptr<T> get_option(char short_opt) const;

protected:
	std::vector<Option_ptr> options_;
	std::string description_;
	std::vector<std::string> non_option_args_;
	std::vector<std::string> unknown_options_;

	Option_ptr find_option(const std::string& long_opt) const;
	Option_ptr find_option(char short_opt) const;
};



class HelpPrinter
{
public:
	HelpPrinter(const OptionParser& option_parser);
	virtual ~HelpPrinter() = default;

	std::string help(const Visibility& max_visibility = Visibility::normal) const;
};




/// Option implementation /////////////////////////////////

inline Option::Option(const std::string& short_option, const std::string& long_option, std::string description) :
	short_option_(short_option),
	long_option_(long_option),
	description_(std::move(description)),
	visibility_(Visibility::normal)
{
	if (short_option.size() > 1)
		throw std::invalid_argument("length of short option must be <= 1: '" + short_option + "'");

	if (short_option.empty() && long_option.empty())
		throw std::invalid_argument("short and long option are empty");
}


inline char Option::short_option() const
{
	if (!short_option_.empty())
		return short_option_[0];
	return 0;
}


inline std::string Option::long_option() const
{
	return long_option_;
}


inline std::string Option::description() const
{
	return description_;
}


inline void Option::set_visibility(const Visibility& visibility)
{
	visibility_ = visibility;
}


inline Visibility Option::visibility() const
{
	return visibility_;
}




/// ValueTemplate implementation /////////////////////////////////

template<class T>
inline ValueTemplate<T>::ValueTemplate(const std::string& short_option, const std::string& long_option, const std::string& description, T* assign_to) :
	Option(short_option, long_option, description),
	count_(0),
	assign_to_(assign_to)
{
	update_reference();
}


template<class T>
inline unsigned int ValueTemplate<T>::count() const
{
	return count_;
}


template<class T>
inline bool ValueTemplate<T>::is_set() const
{
	return (count() > 0);
}


template<class T>
inline void ValueTemplate<T>::assign_to(T* var)
{
	assign_to_ = var;
	update_reference();
}


template<class T>
void ValueTemplate<T>::update_reference()
{
	if ((assign_to_) && is_set())
		*assign_to_ = value();
}


template<class T>
inline void ValueTemplate<T>::add_value(const T& value)
{
	values_.push_back(value);
	++count_;
	update_reference();
}


template<class T>
inline void ValueTemplate<T>::set_value(const T& value)
{
	values_.clear();
	add_value(value);
}


template<class T>
inline T ValueTemplate<T>::value(size_t idx) const
{
	if (!is_set() || (idx >= count_))
	{
		std::stringstream optionStr;
		if (!!is_set())
			optionStr << "option not set: \"";
		else
			optionStr << "index out of range (" << idx << ") for \"";

		if (short_option() != 0)
			optionStr << "-" << short_option();
		else
			optionStr << "--" << long_option();

		optionStr << "\"";
		throw std::out_of_range(optionStr.str());
	}

	return values_[idx];
}




/// Value implementation /////////////////////////////////

template<class T>
inline Value<T>::Value(const std::string& short_option, const std::string& long_option, const std::string& description) :
	ValueTemplate<T>(short_option, long_option, description, nullptr)
{
}


template<class T>
inline Value<T>::Value(const std::string& short_option, const std::string& long_option, const std::string& description, const T& default_val, T* assign_to) :
	ValueTemplate<T>(short_option, long_option, description, assign_to)
{
	set_default(default_val);
}


template<class T>
inline void Value<T>::set_default(const T& value)
{
	this->default_.reset(new T);
	*this->default_ = value;
	update_reference();
}


template<class T>
inline bool Value<T>::has_default() const
{
	return (this->default_ != nullptr);
}


template<class T>
inline T Value<T>::get_default() const
{
	if (!has_default())
		throw std::runtime_error("no default value set");
	return *this->default_;
}


template<class T>
inline void Value<T>::update_reference()
{
	if (this->assign_to_)
	{
		if (this->is_set() || default_)
			*this->assign_to_ = value();
	}
}


template<class T>
inline T Value<T>::value(size_t idx) const
{
	if (!this->is_set() && default_)
		return *default_;
	return ValueTemplate<T>::value(idx);
}


template<class T>
inline Argument Value<T>::argument_type() const
{
	return Argument::required;
}


template<>
inline void Value<std::string>::parse(const std::string& what_option, const char* value)
{
	if (strlen(value) == 0)
		throw std::invalid_argument("missing argument for " + what_option);

	add_value(value);
}


template<class T>
inline void Value<T>::parse(const std::string& what_option, const char* value)
{
	T parsed_value;
	std::string strValue;
	if (value != nullptr)
		strValue = value;

	std::istringstream is(strValue);
	int valuesRead = 0;
	while (is.good())
	{
		if (is.peek() != EOF)
			is >> parsed_value;
		else
			break;

		valuesRead++;
	}

	if (is.fail())
		throw std::invalid_argument("invalid argument for " + what_option + ": '" + strValue + "'");

	if (valuesRead > 1)
		throw std::invalid_argument("too many arguments for " + what_option + ": '" + strValue + "'");

	if (strValue.empty())
		throw std::invalid_argument("missing argument for " + what_option);

	this->add_value(parsed_value);
}


template<class T>
inline void Value<T>::get_help_info(Type& type, bool& has_default, std::string& default_value) const
{
	type = Type::kValue;
	has_default = this->has_default();
	if (has_default)
	{
		std::stringstream default_string;
		default_string << get_default();
		default_value = default_string.str();
	}
}





/// Implicit implementation /////////////////////////////////

template<class T>
inline Implicit<T>::Implicit(const std::string& short_option, const std::string& long_option, const std::string& description, const T& implicit_val, T* assign_to) :
	Value<T>(short_option, long_option, description, implicit_val, assign_to)
{
}


template<class T>
inline Argument Implicit<T>::argument_type() const
{
	return Argument::optional;
}


template<class T>
inline void Implicit<T>::parse(const std::string& what_option, const char* value)
{
	if ((value != nullptr) && (strlen(value) > 0))
		Value<T>::parse(what_option, value);
	else
		this->add_value(*this->default_);
}


template<class T>
inline void Implicit<T>::get_help_info(Type& type, bool& has_default, std::string& default_value) const
{
	type = Type::kImplicit;
	Type placeholder_type;
	Value<T>::get_help_info(placeholder_type, has_default, default_value);
}




/// Switch implementation /////////////////////////////////

inline Switch::Switch(const std::string& short_option, const std::string& long_option, const std::string& description, bool* assign_to) :
	ValueTemplate<bool>(short_option, long_option, description, assign_to)
{
	if (assign_to != nullptr)
		*assign_to = false;
}


inline void Switch::parse(const std::string& /*what_option*/, const char* /*value*/)
{
	add_value(true);
}


inline Argument Switch::argument_type() const
{
	return Argument::no;
}


inline void Switch::get_help_info(Type& type, bool& has_default, std::string& default_value) const
{
	type = Type::kSwitch;
	has_default = false;
}




/// OptionParser implementation /////////////////////////////////

inline OptionParser::OptionParser(std::string description) : description_(std::move(description))
{
}


template<typename T, typename... Ts>
inline std::shared_ptr<T> OptionParser::add(Ts&&... params)
{
	return add<T, Visibility::normal>(std::forward<Ts>(params)...);
}


template<typename T, Visibility visibility, typename... Ts>
inline std::shared_ptr<T> OptionParser::add(Ts&&... params)
{
	static_assert(
		std::is_base_of<Option, typename std::decay<T>::type>::value,
		"type T must be Switch, Value or Implicit"
	);
	std::shared_ptr<T> option = std::make_shared<T>(std::forward<Ts>(params)...);

	for (const auto& o: options_)
	{
		if ((option->short_option() != 0) && (option->short_option() == o->short_option()))
			throw std::invalid_argument("duplicate short option '-" + std::string(1, option->short_option()) + "'");
		if (!option->long_option().empty() && (option->long_option() == (o->long_option())))
			throw std::invalid_argument("duplicate long option '--" + option->long_option() + "'");
	}
	option->set_visibility(visibility);
	options_.push_back(option);
	return option;
}


inline std::string OptionParser::description() const
{
	return description_;
}


inline const std::vector<Option_ptr>& OptionParser::options() const
{
	return options_;
}


inline const std::vector<std::string>& OptionParser::non_option_args() const
{
	return non_option_args_;
}


inline const std::vector<std::string>& OptionParser::unknown_options() const
{
	return unknown_options_;
}


inline Option_ptr OptionParser::find_option(const std::string& long_opt) const
{
	for (const auto& option: options_)
		if (option->long_option() == long_opt)
			return option;
	return nullptr;
}


inline Option_ptr OptionParser::find_option(char short_opt) const
{
	for (const auto& option: options_)
		if (option->short_option() == short_opt)
			return option;
	return nullptr;
}


template<typename T>
inline std::shared_ptr<T> OptionParser::get_option(const std::string& long_opt) const
{
	Option_ptr option = find_option(long_opt);
	if (!option)
		throw std::invalid_argument("option not found: " + long_opt);
	auto result = std::dynamic_pointer_cast<T>(option);
	if (!result)
		throw std::invalid_argument("cannot cast option to T: " + long_opt);
	return result;
}


template<typename T>
inline std::shared_ptr<T> OptionParser::get_option(char short_opt) const
{
	Option_ptr option = find_option(short_opt);
	if (!option)
		throw std::invalid_argument("option not found: " + std::string(1, short_opt));
	auto result = std::dynamic_pointer_cast<T>(option);
	if (!result)
		throw std::invalid_argument("cannot cast option to T: " + std::string(1, short_opt));
	return result;
}


inline void OptionParser::parse(int argc, const char * const * argv)
{
	unknown_options_.clear();
	non_option_args_.clear();
	for (int n=1; n<argc; ++n)
	{
		const std::string arg(argv[n]);
		if (arg == "--")
		{
			///from here on only non opt args
			for (int m=n+1; m<argc; ++m)
				non_option_args_.push_back(argv[m]);

			break;
		}
		else if (arg.find("--") == 0)
		{
			/// long option arg
			std::string opt = arg.substr(2);
			std::string optarg;
			size_t equalIdx = opt.find('=');
			if (equalIdx != std::string::npos)
			{
				optarg = opt.substr(equalIdx + 1);
				opt.resize(equalIdx);
			}

			Option_ptr option = find_option(opt);
			if (option && (option->visibility() == Visibility::inactive))
				option = nullptr;
			if (option)
			{
				if (option->argument_type() == Argument::no)
				{
					if (!optarg.empty())
						option = nullptr;
				}
				else if (option->argument_type() == Argument::required)
				{
					if (optarg.empty() && n < argc-1)
						optarg = argv[++n];
				}
			}

			if (option)
				option->parse(opt, optarg.c_str());
			else
				unknown_options_.push_back(arg);
		}
		else if (arg.find("-") == 0)
		{
			/// short option arg
			std::string opt = arg.substr(1);
			bool unknown = false;
			for (size_t m=0; m<opt.size(); ++m)
			{
				char c = opt[m];
				std::string optarg;

				Option_ptr option = find_option(c);
				if (option && (option->visibility() == Visibility::inactive))
					option = nullptr;
				if (option)
				{
					if (option->argument_type() == Argument::required)
					{
						/// use the rest of the current argument as optarg
						optarg = opt.substr(m + 1);
						/// or the next arg
						if (optarg.empty() && n < argc-1)
							optarg = argv[++n];
						m = opt.size();
					}
					else if (option->argument_type() == Argument::optional)
					{
						/// use the rest of the current argument as optarg
						optarg = opt.substr(m + 1);
						m = opt.size();
					}
				}

				if (option)
					option->parse(std::string(1, c), optarg.c_str());
				else
					unknown = true;
			}
			if (unknown)
				unknown_options_.push_back(arg);
		}
		else
		{
			non_option_args_.push_back(arg);
		}
	}
}




inline std::string OptionParser::help(const Visibility& max_visibility) const
{
	if (max_visibility < Visibility::normal)
		throw std::invalid_argument("visibility must be at least normal");

	std::stringstream s;
	if (!description_.empty())
		s << description_ << ":\n";

/*	size_t optionRightMargin(20);
	const size_t maxDescriptionLeftMargin(40);
*/
//	const size_t descriptionRightMargin(80);


/*
inline std::string Option::to_string() const
{
	std::stringstream line;
	if (short_option() != 0)
	{
		line << "  -" << short_option();
		if (!long_option().empty())
			line << ", ";
	}
	else
		line << "  ";

	if (!long_option().empty())
		line << "--" << long_option();

	return line.str();
}

inline std::string Value<T>::to_string() const
{
	std::stringstream ss;
	ss << Option::to_string() << " arg";
	if (default_)
	{
		std::stringstream defaultStr;
		defaultStr << *default_;
		if (!defaultStr.str().empty())
			ss << " (=" << *default_ << ")";
	}
	return ss.str();
}

template<class T>
inline std::string Implicit<T>::to_string() const
{
	std::stringstream ss;
	ss << Option::to_string() << " [=arg(=" << *this->default_ << ")]";
	return ss.str();
}

inline std::string Switch::to_string() const
{
	return Option::to_string();
}

*/


/*
	for (const auto& option: options_)
		optionRightMargin = std::max(optionRightMargin, option->to_string().size() + 2);
	optionRightMargin = std::min(maxDescriptionLeftMargin - 2, optionRightMargin);

	for (const auto& option: options_)
	{
		if ((option->visibility() <= Visibility::hidden) || 
			(option->visibility() > max_visibility))
			continue;
		std::string optionStr = option->to_string();

		if (option->type() == Type::kImplicit)
		{
			std::cout << "Implicit: ";
		}
		else if (option->type() == Type::kSwitch)
		{
			std::cout << "Switch:   ";
		}
		else if (option->type() == Type::kValue)
		{
			std::cout << "Value:    ";
		}

		if (option->short_option() != 0)
		{
			std::cout << "  -" << option->short_option();
			if (!option->long_option().empty())
				std::cout << ", ";
		}
		else
			std::cout << "  ";

		if (!option->long_option().empty())
			std::cout << "--" << option->long_option();

		if (option->has_default())
			std::cout << Option::to_string() << " [=arg(=" << *this->default_ << ")]";

		std::cout << "\n";



		if (optionStr.size() < optionRightMargin)
			optionStr.resize(optionRightMargin, ' ');
		else
			optionStr += "\n" + std::string(optionRightMargin, ' ');
		s << optionStr;

		std::string line;
		std::vector<std::string> lines;
		std::stringstream description(option->description());
		while (std::getline(description, line, '\n'))
			lines.push_back(line);

		std::string empty(optionRightMargin, ' ');
		for (size_t n=0; n<lines.size(); ++n)
		{
			if (n > 0)
				s << "\n" << empty;
			s << lines[n];
		}
		s << "\n";
	}

	return s.str();
*/
	return "";
}


HelpPrinter::HelpPrinter(const OptionParser& option_parser)
{

}


std::string HelpPrinter::help(const Visibility& max_visibility) const
{

}



static std::ostream& operator<<(std::ostream& out, const OptionParser& op)
{
	out << op.help();
	return out;
}


} // namespace popl


#endif // POPL_HPP


