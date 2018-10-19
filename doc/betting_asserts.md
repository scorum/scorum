# Betting operations exceptions
List of exception that could be raised on betting operations

## create_game_operation
* Assert Exception "Account \"${1}\" must exist." -- raises when account with name op.moderator doens't exists
* Assert Exception "Game should start after head block time" -- raises when operation arg start_time <= head_block_time
* Assert Exception "Game can only be resolved after it has been started" -- raises when op.start_time <= op.auto_resolve_delay_sec + head_block_time
* Assert Exception "Game with name '$(op.name)' already exists -- raises when game with same uid already exists
* Assert Exception "User '$(op.moderator)' isnt a betting moderator" -- raises when try to create game by non-betting_moderator account

## update_game_start_time_operation
* Assert Exception "Account \"${1}\" must exist." -- raises when account with name op.moderator doens't exists 
* Assert Exception "Game should start after head block time" -- raises when operation arg start_time <= head_block_time
* Assert Exception "User '$(op.moderator)' isnt a betting moderator" -- raises when try to update game by non-betting_moderator account
* Assert Exception "Game with uid '${op.uid}' doesn't exist" -- raises when there is no game with passed uid
* Assert Exception "Cannot change the start time when game is finished" raises when try to update game which status is "finished"(status changed to finished after game.start_time + game.auto_resolve_delay_sec was reached)
* Assert Exception "Cannot change start time more than ${1} seconds" -- raises when try to move game start time > than SCORUM_BETTING_START_TIME_DIFF_MAX value

## update_game_markets operation
* Assert Exception "Account \"${1}\" must exist." -- raises when account with name op.moderator doens't exists
* Assert Exception "User ${op.moderator} isn't a betting moderator" -- raises when try to update game by non-betting_moderator account
* Assert Exception "Game with uid '${op.uid}' doesn't exist" -- raises when there is no game with passed uid
* Assert Exception "Cannot change the markets when game is finished" raises when try to update game with status "finished"(status changed to finished after game.start_time + game.auto_resolve_delay_sec was reached)
* Assert Exception "Cannot cancel markets after game was started" -- raises when try to remove markets from game which status is "started" (status changes to "started" after game start_time was reached)

## post_game_results_operation
* Assert Exception "Account \"${1}\" must exist." -- raises when account with name op.moderator doens't exists
* Assert Exception "User '$(op.creator)' isnt a betting moderator" -- raises when try to post game results by non-betting_moderator account
* Assert Exception "Game with uid '${op.uid}' doesn't exist" -- raises when there is no game with passed uid
* Assert Exception "The game is not started yet" -- raises when try to post results for game which status is "created" (status changes to "started" after game start_time was reached)
* Assert Exception "Unable to post game results after bets were resolved" -- raises when try to post results op.start_time + op.auto_resolve_delay_sec was reached
* Assert Exception "Wincase winners list do not contain neither '${1}' nor '${2}'" 
* Assert Exception "You've provided opposite wincases from same market as winners"  

## cancel_game_operation
* Assert Exception "Account \"${1}\" must exist." -- raises when account with name op.moderator doens't exists
* Assert Exception "User '$(op.moderator)' isnt a betting moderator" -- raises when try to cancel game by non-betting_moderator account
* Assert Exception "Game with uid '${op.uid}' doesn't exist" -- raises when there is no game with passed uid
* Assert Exception "Cannot cancel the game after it is finished" -- raises when try to cancel game which status is finished(status changed to finished after game.start_time + game.auto_resolve_delay_sec was reached)

## post_bet_operation
* Assert Exception "Account \"${1}\" must exist." -- raises when account with name op.better doens't exists
* Assert Exception "Game with uuid ${1} doesn't exist" -- raises when there is no game with passed uid
* Assert Exception "Cannot post bet for game that is finished" -- raises when try post bet on game which status is "finished"(status changed to finished after game.start_time + game.auto_resolve_delay_sec was reached)
* Assert Exception "Cannot create non-live bet after game was started" -- raises when try to post bet with arg bet.live == False on game which status is "started"(status changes to "started" after game start_time was reached))
* Assert Exception "Invalid wincase '${w}'" -- raises when try to post bet on invalid wincase

## cancel_pending_bet
* Assert Exception "Account \"${1}\" must exist." -- raises when account with name op.better doens't exists 
* Assert Exception "Bet ${1} doesn't exist" -- raises when there is no bet with passed uid
* Assert Exception "Invalid better" -- raises when operation.better != bet.better
