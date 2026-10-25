# Screenshots

| File | What it shows |
|---|---|
| `01_menu_and_products.png` | Main menu and the product list |
| `02_cart_and_checkout.png` | Add to cart, view cart, place order, cart emptied afterwards |
| `03_stock_rollback.png` | Checkout rejected because stock dropped; transaction rolled back, nothing changed |
| `04_reports.png` | Order history, low-stock report, total sales, top-selling products |

These images are renderings of the **real output** of the program running against MySQL 8.0
(blue = prompt, yellow = what was typed). The main menu is shown once and trimmed from the
later images to keep them readable. The full text is in [`docs/sample_session.txt`](../docs/sample_session.txt).

Want screenshots from your own terminal instead? Run `./cartforge` and replace these files
using the same names - the README links will keep working.
