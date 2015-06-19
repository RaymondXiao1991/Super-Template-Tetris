#pragma once

#include "block_generator.h"
#include "direction.h"
#include "input.h"
#include "grid.h"
#include "printer.h"
#include "random.h"
#include "serialize.h"
#include "playfield.h"
#include "buffer.h"

/**
    Seed for the random number generator.
*/
template <unsigned max>
using InitialRandom = LinearGenerator<uint32_t, 12345>;

/**
    General state of the player
*/
enum class PlayerState : unsigned
{
    Alive,
    Dead
};

/**
    State of a snake game.
*/
template <
    PlayerState currentPlayerState,
    unsigned currentScore,
    typename currentPosition,
    typename currentBlock,
    typename currentWorld,
    typename currentBlockGenerator>
class State
{
    using nextBlock = typename currentBlockGenerator::next::value;

public:
    static constexpr const PlayerState playerState = currentPlayerState;
    static constexpr const unsigned score = currentScore;
    
    using position = currentPosition;
    using block = currentBlock;
    
    using world = currentWorld;
    
    using random = currentBlockGenerator;
    
    template <typename newPos>
    using set_position = State<playerState, score, newPos, block, world, random>;
    
    template <typename newWorld>
    using set_world = State<playerState, score, position, block, newWorld, random>;
    
    template <typename newRandom>
    using set_random = State<playerState, score, position, block, world, newRandom>;
    
    template <typename newBlock>
    using set_block = State<playerState, score, position, newBlock, world, random>;
    
    using place_initial_piece = State<
        playerState,
        score,
        Position<
            (currentWorld::width / 2) - (nextBlock::pieces::width) / 2,
            0>,
        nextBlock,
        world,
        typename random::next>;
};

/**
    The initial state of a snake game.
*/
using InitialState =
    typename State<
        PlayerState::Alive,
        0,
        Position<0, 0>,
        typename initialBlockGenerator::value,
        InitialWorld,
        initialBlockGenerator>::place_initial_piece;

/**
    Progress the game by one step by consuming one input.
*/
template <
    Input input,
    typename state>
struct step {
   using type = typename state::template set_position<typename state::position::template add<Position<0, 1>>>;
};

/**
    For the case where we are dead, noop.
*/
template <
    Input input,
    unsigned score,
    typename position,
    typename block,
    typename world,
    typename blockGenerator>
struct step<input, State<PlayerState::Dead, score, position, block, world, blockGenerator>> {
    using type = State<PlayerState::Dead, score, position, block, world, blockGenerator>;
};

template <Input input, typename state>
using step_t = typename step<input, state>::type;


/*------------------------------------------------------------------------------
    Printer
*/
template <
    PlayerState playerState,
    unsigned score,
    typename position,
    typename block,
    typename world,
    typename blockGenerator>
struct Printer<State<playerState, score, position, block, world, blockGenerator>>
{
    // Draw outline
    using initial_buffer = buffer_draw_rect_outline<
        Position<0, 0>,
        world::width + 2,
        world::height + 2,
        '+',
        default_gfx,
        empty_buffer<world::width + 2, world::height + 2>>;
    
    // Draw death area
    using death_buffer = buffer_draw_rect<
        Position<1, 1>,
        world::width,
        2,
        '-',
        default_gfx,
        initial_buffer>;
    
    struct xxxx {
        template <typename x>
        using apply = identity<typename to_px<x>::type>;
    };
    
    // draw playfield
    using play_buffer = buffer_draw_grid<
        Position<1, 1>,
        fmap_t<xxxx, world>,
        death_buffer>;

    // Draw current block
    using buffer = buffer_draw_grid<
        Position<1, 1>::add<position>,
        fmap_t<xxxx, typename block::pieces>,
        play_buffer>;

    static void Print(std::ostream& output)
    {
        output << "Score:" << score;
        output << " -- " << (playerState == PlayerState::Dead ? " You Are Dead " : "");
        output << "\n";
        Printer<buffer>::Print(output);
    }
};

/*------------------------------------------------------------------------------
    Serialize
*/
template <PlayerState state>
struct Serialize<SerializableValue<PlayerState, state>>
{
    static std::ostream& Write(std::ostream& output)
    {
        output << "PlayerState::";
        switch (state)
        {
        case PlayerState::Alive: output << "Alive"; break;
        case PlayerState::Dead: output << "Dead"; break;
        }
        return output;
    }
};

template <
    PlayerState playerState,
    unsigned score,
    typename position,
    typename block,
    typename world,
    typename blockGenerator>
struct Serialize<State<playerState, score, position, block, world, blockGenerator>>
{
    static std::ostream& Write(std::ostream& output)
    {
        output << "State<";
        Join<',',
            SerializableValue<PlayerState, playerState>,
            SerializableValue<unsigned, score>,
            block,
            world,
            blockGenerator>::Write(output);
        return output << ">";
    }
};