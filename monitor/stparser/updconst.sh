#!/bin/bash
output_path=.
if [[ ! -z "$1" ]]; then
    output_path="$1"
fi
stateNo=`awk '/State ([0-9]{3}|[0-9]{2}|[0-9])/ { last_match = $2; } /cinner_statements_1.*\.  \[.*TOK_VCASE_SEP/ { printf("case %d: ", last_match);}' ${output_path}/stparser.output`
tempNo=${stateNo#case};stateNo=${tempNo%: }
sed -i "s/define CONFLICT_STATE .*/define CONFLICT_STATE     $stateNo/" parsrctx.h
echo CONFLICT_STATE=$stateNo

stateNo=`awk '/State ([0-9]{3}|[0-9]{2}|[0-9])/ { last_match = $2; } /arithmetic_expr: arithmetic_expr \. TOK_ADD/ { print last_match; exit }' ${output_path}/stparser.output`
sed -i "s/define SUBEXPR_STATE .*/define SUBEXPR_STATE     $stateNo/" parsrctx.h
echo SUBEXPR_STATE=$stateNo

stateNo=`awk '/State ([0-9]{3}|[0-9]{2}|[0-9])/ { last_match = $2; } /l_value \. TOK_VPUNC TOK_NUMBER/ { printf( "case %d: ",last_match ); }' ${output_path}/stparser.output`
tempNo=${stateNo#case};stateNo=${tempNo%: }
sed -i "s/define LVALUE_BIT_STATES .*/define LVALUE_BIT_STATES     $stateNo/" parsrctx.h
echo LVALUE_BIT_STATES=$stateNo

stateNo=`awk '/State ([0-9]{3}|[0-9]{2}|[0-9])/ { last_match = $2; } /TOK_METHOD .*instance_path \. TOK_VSEMICOLON/ { printf( "%d, ",last_match ); }' ${output_path}/stparser.output`
stateNo="${stateNo%, }"
sed -i "s/define METHOD_RET_STATE.*/define METHOD_RET_STATE     $stateNo/" parsrctx.h
echo METHOD_RET_STATE=$stateNo

stateNo=`awk '/State ([0-9]{3}|[0-9]{2}|[0-9])/ { last_match = $2; } /TOK_USING instance_path \. TOK_VSEMICOLON/ { printf( "%d, ",last_match ); }' ${output_path}/stparser.output`
stateNo="${stateNo%, }"
sed -i "s/define USING_SEP_STATE .*/define USING_SEP_STATE     $stateNo/" parsrctx.h
echo USING_SEP_STATE=$stateNo

stateNo=`awk '/State ([0-9]{3}|[0-9]{2}|[0-9])/ { last_match = $2; } /TOK_VSTART_CASESEL case_check_statement \. error/ { printf( "%d, ",last_match ); }/TOK_VSTART_CASESEL case_list_selector \./ { printf( "%d, ",last_match ); }' ${output_path}/stparser.output`
stateNo="${stateNo%, }"
sed -i "s/define CASESEL_CHECK_STATES .*/define CASESEL_CHECK_STATES     std::vector<int>({ ${stateNo} })/" parsrctx.h
echo CASESEL_CHECK_STATES=$stateNo;
