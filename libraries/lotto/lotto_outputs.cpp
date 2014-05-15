#include <bts/lotto/lotto_outputs.hpp>


namespace bts { namespace lotto {
    const operation_type_enum secret_operation::type = operation_type_enum::secret_op_type;
    const operation_type_enum ticket_operation::type = operation_type_enum::ticket_op_type;
    const operation_type_enum jackpot_operation::type = operation_type_enum::jackpot_op_type;
}} // bts::lotto
