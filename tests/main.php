<?php
$b = new bitmap(1024);
echo "bit 1\n";
$b->set(1, 0);
var_dump($b->get(1));

echo "bit 1\n";
$b->set(1, 1);
var_dump($b->get(1));

echo "bit 7\n";
$b->set(7, 1);
var_dump($b->get(7));
var_dump($b->get(0));

echo "bit 8\n";
$b->set(8, 1);
var_dump($b->get(9));
var_dump($b->get(8));
var_dump($b->get(7));
var_dump($b->get(0));
