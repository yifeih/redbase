create table customers(
      clubcardnum int,
      name c15,
      age int );

create table items(
      itemid int,
      name c15,
      category c20 );

create table stores(
      city c20,
      storetype c1,
      squarefootage float );

create table purchases(
      customerclubcard int,
      itemid int,
      storecity c20,
      quantity int,
      purchaseid int );


