#1600
door test~
0 d 100
pizza~
say Hi there.
eval room %self.room%
%echo% ROOM: %room%
%echo% CHAR.ROOM: %self.room%
~
#1615
No Entry~
0 g 100
~
   if %actor.name% != zizazat
  wait 2
  emote snaps to attention.
  wait 3
  say None are welcome in this domain!
else
  say We have been awaiting your return, m'lord.
  wait 1s
  unlock iron
  open iron
  wait 4s
  close iron
  lock iron
  emote snaps back to attention.
end
~
#1618
dog test~
0 dg 100
bark~
emote barks like a dog.
~
$~
