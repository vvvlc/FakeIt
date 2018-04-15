#pragma once

#include "fakeit/DefaultFakeit.hpp"
#include "fakeit/EventHandler.hpp"
#include "mockutils/to_string.hpp"
#include "catch.hpp"

#define BACKWARD_HAS_BFD 1
#include <backward.hpp>

namespace fakeit {

    struct VerificationException : public FakeitException {
        virtual ~VerificationException() = default;

        void setFileInfo(const char *file, int line, const char *callingMethod) {
            _file = file;
            _callingMethod = callingMethod;
            _line = line;
        }

        const char *file() const {
            return _file;
        }

        int line() const {
            return _line;
        }

        const char *callingMethod() const {
            return _callingMethod;
        }

    private:
        const char *_file;
        int _line;
        const char *_callingMethod;
    };

    struct NoMoreInvocationsVerificationException : public VerificationException {

        NoMoreInvocationsVerificationException(std::string format) : //
                _format(format) { //
        }

        virtual std::string what() const override {
            return _format;
        }

    private:
        std::string _format;
    };

    struct SequenceVerificationException : public VerificationException {
        SequenceVerificationException(const std::string &format) : //
                _format(format) //
        {
        }

        virtual std::string what() const override {
            return _format;
        }

    private:
        std::string _format;
    };

    class CatchAdapter : public EventHandler {
        EventFormatter &_formatter;

        std::string formatLineNumber(std::string file, int num) {
#ifndef __GNUG__
            return file + std::string("(") + fakeit::to_string(num) + std::string(")");
#else
            return file + std::string(":") + fakeit::to_string(num);
#endif
        }

    public:

        virtual ~CatchAdapter() = default;

        CatchAdapter(EventFormatter &formatter)
                : _formatter(formatter) {}

        void fail(
                std::string vetificationType,
                Catch::SourceLineInfo sourceLineInfo,
                std::string failingExpression,
                std::string fomattedMessage,
                Catch::ResultWas::OfType resultWas = Catch::ResultWas::OfType::ExpressionFailed ){
            Catch::AssertionHandler catchAssertionHandler( vetificationType, sourceLineInfo, failingExpression, Catch::ResultDisposition::Normal );
            INTERNAL_CATCH_TRY { \
                CATCH_INTERNAL_SUPPRESS_PARENTHESES_WARNINGS \
                catchAssertionHandler.handleMessage(resultWas, fomattedMessage); \
                CATCH_INTERNAL_UNSUPPRESS_PARENTHESES_WARNINGS \
            } INTERNAL_CATCH_CATCH(catchAssertionHandler) { \
                INTERNAL_CATCH_REACT(catchAssertionHandler) \
            }
        }

        bool hasEnding (std::string const &fullString, std::string const &ending) {
            if (fullString.length() >= ending.length()) {
                return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
            } else {
                return false;
            }
        }

        virtual void handle(const UnexpectedMethodCallEvent &evt) override {
        	using namespace backward;
        	size_t failedFrame=SIZE_MAX;
        	StackTrace st; st.load_here(32);
        	Printer p;
        	p.object = false;
        	p.color_mode = ColorMode::always;
        	p.address = false;
        	p.snippet = false;
        	//p.print(st, stdout);

        	std::ostringstream stout;
        	TraceResolver tr; tr.load_stacktrace(st);

        	size_t main_i=SIZE_MAX;

        	//find main(int argc, char *argv[]
        	for (size_t i = st.size()-1; i > 0; --i) {
        		ResolvedTrace trace = tr.resolve(st[i]);

        		if (trace.object_function.compare("main")==0) {
        			main_i=i;
        			break;
        		}
        	}


        	for (size_t i = 0; i < ((main_i==SIZE_MAX)?st.size():main_i); ++i) {
        		ResolvedTrace trace = tr.resolve(st[i]);

        		if (!hasEnding(trace.source.filename, std::string(__FILE__)) &&
        			!hasEnding(trace.source.filename, std::string("backward.hpp")) &&
        			!hasEnding(trace.source.filename, std::string("catch222.hpp"))
        					) {

        			//identify failed frame
        			if (failedFrame == SIZE_MAX) {
        				failedFrame = i;
        			}
        			stout << "     " << trace.source.filename << ":" << trace.source.line << " " << trace.object_function << std::endl;

        		}
        	}
        	if (failedFrame == SIZE_MAX) {
        		std::string format = _formatter.format(evt);
        		fail("UnexpectedMethodCall",::Catch::SourceLineInfo("Unknown file",0),"",format, Catch::ResultWas::OfType::ExplicitFailure);
        	} else {
        		//TODO: move this to UnknownMethod::instnacne (add stactka trace and replace unknown method)
        		using namespace std;
        		ResolvedTrace trace = tr.resolve(st[failedFrame]);
        		SnippetFactory _snippets;

				typedef SnippetFactory::lines_t lines_t;

				lines_t lines = _snippets.get_snippet(trace.source.filename, trace.source.line, 1);
        		std::string format = _formatter.format(evt);

        		std::ostringstream out;
                out << format << std::endl;
                if (lines.size()>0) {
                	out << "  Failed statement: " << std::endl;
                	out << "  " << lines.at(0).second << std::endl;
                	//out << std::string( trace.source.col, ' ' ) << "^" << std::endl;
                	out << "  Stacktrace: " << std::endl;
                	out << stout.str() << std::endl;
                }
                format=out.str();
        		fail("UnexpectedMethodCall",::Catch::SourceLineInfo(trace.source.filename.c_str(), trace.source.line),"",format, Catch::ResultWas::OfType::ExplicitFailure);
        	}
        }

        virtual void handle(const SequenceVerificationEvent &evt) override {
            std::string format(formatLineNumber(evt.file(), evt.line()) + ": " + _formatter.format(evt));
            std::string expectedPattern {DefaultEventFormatter::formatExpectedPattern(evt.expectedPattern())};
            fail("Verify",::Catch::SourceLineInfo(evt.file(),evt.line()),expectedPattern,format);
        }


        virtual void handle(const NoMoreInvocationsVerificationEvent &evt) override {
            std::string format(formatLineNumber(evt.file(), evt.line()) + ": " + _formatter.format(evt));
            fail("VerifyNoMoreInvocations",::Catch::SourceLineInfo(evt.file(),evt.line()),"",format);
        }

    };


    class CatchFakeit : public DefaultFakeit {


    public:

        virtual ~CatchFakeit() = default;

        CatchFakeit() : _formatter(), _catchAdapter(_formatter) {}

        static CatchFakeit &getInstance() {
            static CatchFakeit instance;
            return instance;
        }

    protected:

        fakeit::EventHandler &accessTestingFrameworkAdapter() override {
            return _catchAdapter;
        }

        EventFormatter &accessEventFormatter() override {
            return _formatter;
        }

    private:

        DefaultEventFormatter _formatter;
        CatchAdapter _catchAdapter;
    };

}
