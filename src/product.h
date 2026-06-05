#ifndef PRODUCT_H
#define PRODUCT_H

#include <mysql.h>

class Product
{
private:
    MYSQL *conn;

public:
    Product(MYSQL *connection);

    void addProduct();
    void viewProducts();
    void searchProduct();
    void updateStock();
    void deleteProduct();
};

#endif