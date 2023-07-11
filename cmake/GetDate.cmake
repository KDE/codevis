macro(GET_DATE RESULT)
	if(WIN32)
		execute_process(COMMAND "cmd" " /C date /T" OUTPUT_VARIABLE ${RESULT})
		string(REGEX REPLACE "(..)/(..)/(....).*" "\\3-\\2-\\1" ${RESULT}
${${RESULT}})
	elseif(UNIX)
		execute_process(COMMAND "date" "+%Y-%m-%d" OUTPUT_VARIABLE ${RESULT})
	else()
		message(SEND_ERROR "Unable to detect date")
		set(${RESULT} UNKNOWN)
	endif()
endmacro()

