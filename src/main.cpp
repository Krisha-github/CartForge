#include <iostream>
#include "db_connection.h"
#include "product.h"
#include "user.h"
#include "cart.h"
#include "order.h"

using namespace std;

int main()
{
    DBConnection db;
    MYSQL *conn = db.connect();

    if (!conn)
    {
        cout << "Could not connect to database. Exiting.\n";
        return 1;
    }

    Product product(conn);
    User user(conn);
    Cart cart(conn);
    OrderManager orderManager(conn);

    int choice;

    do
    {
        cout << "\n========== CartForge ==========\n";
        cout << "1. Add Product\n";
        cout << "2. View Products\n";
        cout << "3. Search Product\n";
        cout << "4. Update Product Stock\n";
        cout << "5. Delete Product\n";
        cout << "6. Add User\n";
        cout << "7. View Users\n";
        cout << "8. Add Product to Cart\n";
        cout << "9. View Cart\n";
        cout << "10. Remove Item from Cart\n";
        cout << "11. Place Order\n";
        cout << "12. View Order History\n";
        cout << "13. Low Stock Report\n";
        cout << "14. Total Sales Report\n";
        cout << "15. Top Selling Products\n";
        cout << "0. Exit\n";
        cout << "Enter choice: ";
        cin >> choice;

        switch (choice)
        {
        case 1:
            product.addProduct();
            break;
        case 2:
            product.viewProducts();
            break;
        case 3:
            product.searchProduct();
            break;
        case 4:
            product.updateStock();
            break;
        case 5:
            product.deleteProduct();
            break;
        case 6:
            user.addUser();
            break;
        case 7:
            user.viewUsers();
            break;
        case 8:
            cart.addToCart();
            break;
        case 9:
            cart.viewCart();
            break;
        case 10:
            cart.removeFromCart();
            break;
        case 11:
            orderManager.placeOrder();
            break;
        case 12:
            orderManager.viewOrderHistory();
            break;
        case 13:
            orderManager.lowStockReport();
            break;
        case 14:
            orderManager.totalSalesReport();
            break;
        case 15:
            orderManager.topSellingProducts();
            break;
        case 0:
            cout << "Exiting CartForge.\n";
            break;
        default:
            cout << "Invalid choice.\n";
        }

    } while (choice != 0);

    mysql_close(conn);
    return 0;
}