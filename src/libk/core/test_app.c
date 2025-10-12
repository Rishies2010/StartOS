//Test elf execution

int main(void)
{
    int result = 0;
    for (int i = 1; i <= 10; i++)
    {
        result += i;
    }
    if (result == 55)
    {
        return 100;
    }
    else
    {
        return 999;
    }
}