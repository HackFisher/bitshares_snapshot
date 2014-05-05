#include <bts/lotto/lotto_outputs.hpp>


namespace bts { namespace lotto {
    const claim_type_enum claim_ticket_input::type = claim_type_enum::claim_ticket;
    const claim_type_enum claim_ticket_output::type = claim_type_enum::claim_ticket;
    const claim_type_enum claim_secret_input::type = claim_type_enum::claim_secret;
    const claim_type_enum claim_secret_output::type = claim_type_enum::claim_secret;
    const claim_type_enum claim_jackpot_input::type = claim_type_enum::claim_jackpot;
    const claim_type_enum claim_jackpot_output::type = claim_type_enum::claim_jackpot;
}} // bts::lotto
