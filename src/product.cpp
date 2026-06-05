#include "product.h"
#include <iostream>
#include <string>
using namespace std;

Product::Product(MYSQL *connection)
{
    conn = connection;
}

void Product::addProduct()
{
    string name;
    double price;
    int stock;

    cin.ignore();
    cout << "Enter product name: ";
    getline(cin, name);
    cout << "Enter price: ";
    cin >> price;
    cout << "Enter stock quantity: ";
    cin >> stock;

    string query = "INSERT INTO products (product_name, price, stock_quantity) VALUES ('" +
                   name + "', " + to_string(price) + ", " + to_string(stock) + ")";

    if (mysql_query(conn, query.c_str()) == 0)
        cout << "Product added successfully.\n";
    else
        cout << "Error: " << mysql_error(conn) << endl;
}

void Product::viewProducts()
{
    string query = "SELECT product_id, product_name, price, stock_quantity FROM products";

    if (mysql_query(conn, query.c_str()) != 0)
    {
        cout << "Error: " << mysql_error(conn) << endl;
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row;

    cout << "\nID | Product | Price | Stock\n";
    cout << "--------------------------------\n";

    while ((row = mysql_fetch_row(res)))
    {
        cout << row[0] << " | " << row[1] << " | " << row[2] << " | " << row[3] << endl;
    }

    mysql_free_result(res);
}

void Product::searchProduct()
{
    string keyword;

    cin.ignore();
    cout << "Enter product keyword: ";
    getline(cin, keyword);

    string query = "SELECT product_id, product_name, price, stock_quantity FROM products "
                   "WHERE product_name LIKE '%" +
                   keyword + "%'";

    if (mysql_query(conn, query.c_str()) != 0)
    {
        cout << "Error: " << mysql_error(conn) << endl;
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(res)))
    {
        cout << row[0] << " | " << row[1] << " | " << row[2] << " | " << row[3] << endl;
    }

    mysql_free_result(res);
}

void Product::updateStock()
{
    int productId, stock;

    cout << "Enter product ID: ";
    cin >> productId;
    cout << "Enter new stock quantity: ";
    cin >> stock;

    string query = "UPDATE products SET stock_quantity = " + to_string(stock) +
                   " WHERE product_id = " + to_string(productId);

    if (mysql_query(conn, query.c_str()) == 0)
        cout << "Stock updated successfully.\n";
    else
        cout << "Error: " << mysql_error(conn) << endl;
}

void Product::deleteProduct()
{
    int productId;

    cout << "Enter product ID to delete: ";
    cin >> productId;

    string query = "DELETE FROM products WHERE product_id = " + to_string(productId);

    if (mysql_query(conn, query.c_str()) == 0)
        cout << "Product deleted successfully.\n";
    else
        cout << "Error: " << mysql_error(conn) << endl;
}