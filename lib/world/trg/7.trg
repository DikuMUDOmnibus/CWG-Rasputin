#700
draw a card~
2 c 100
draw~
set CARDOBJ 700
eval inroom %self%
eval obj %inroom.contents%
while %obj%
  if %obj.vnum% == %CARDOBJ%
    set CARDISHERE 1
  end
  if %CARDISHERE%
    %send% %actor% There is already a face up Adventure card here!
  halt
  else
    %send% %actor% You reach into the Adventure deck and select the top card...
    %echoaround% %actor% %actor.name% reaches into the Adventure deck and selects the top card...
    %load% obj %CARDOBJ%
  end
  set next_obj %obj.net_in_list%
  set obj %next_obj%
done
~
$~
