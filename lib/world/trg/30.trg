#3040
Kortaal Gate Guard~
0 n 100
~
while (1)
if %time.hour% > 4 && %time.hour% < 21
  wait until 4:00
    say I hereby declare Kortaal open!
    wait 5s
    unlock gate
    open gate
else
  wait until 21:00
    say I hereby declare Kortaal closed!
    wait 5s
    close gate
    lock gate
end
  
done
~
#3049
Sail from Kortaal to Maaken~
0 b 100
~
wait until 7:00
  mat 3049 say The ship will be departing for Maaken in 1 minute!
  mat 3037 say The ship will be departing for Maaken in 1 minute!
  wait 60s
  mat 3049 close ship
  mat 3037 close plank
  drive s
  wait 2s
  drive e
  wait 2s
  drive s
  wait 2s
  drive s
  wait 2s
  drive s
  wait 2s
  drive s
  wait 2s
  drive s
  wait 2s
  drive s
  wait 2s
  drive s
  wait 2s
  drive s
  wait 2s
  drive e
  wait 2s
  drive s
  wait 2s
  drive s
  wait 2s
  mat 3037 say We've arrived in Maaken!
  mat 3037 open plank
  mat 11009 open ship
~
#3050
Sail from Maaken to Kortaal~
0 b 100
~
wait until 11:00
  mat 11009 say The ship will be departing for Kortaal in 1 minute!
  mat 3037 say The ship will be departing for Kortaal in 1 minute!
  wait 60s
  mat 11009 close ship
  mat 3037 close plank
  drive n
  wait 2s
  drive n
  wait 2s
  drive w
  wait 2s
  drive n
  wait 2s
  drive n
  wait 2s
  drive n
  wait 2s
  drive n
  wait 2s
  drive n
  wait 2s
  drive n
  wait 2s
  drive n
  wait 2s
  drive n
  wait 2s
  drive w
  wait 2s
  drive n
  wait 2s
  mat 3037 say We've arrived in Kortaal!
  mat 3037 open plank
  mat 3049 open ship
~
$~
