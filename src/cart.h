#ifndef CART_H
#define CART_H

#include <mysql.h>

class Cart
{
private:
    MYSQL *conn;

public:
    explicit Cart(MYSQL *connection);

    void addToCart();
    void viewCart();
    void removeFromCart();
};

#endif
