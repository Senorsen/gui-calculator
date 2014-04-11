#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "header.h"
#define max(x, y) (x > y ? x : y)
#define min(x, y) (x > y ? y : x)

const long base = 10000;

char expr[10000], outputFmt[50];
int i, j, eps = 3, degreeType = 1, errorDivideByZero, errorExpr, errorTooLarge;

void setValue(bigintegerP b, long x){
    memset(b->dig, 0, sizeof b->dig);
    b->dig[0] = 1;
    b->dig[1] = x % base;
    if(x >= base){
        b->dig[0] = 2;
        b->dig[2] = x / base;
    }
}

double convertToDouble(bigintegerP b){
    double res = 0;
    for(i = b->dig[0]; i; --i)
        res = res * base + b->dig[i];
    return res;
}

biginteger add(bigintegerP b, bigintegerP n){
    biginteger res;
    setValue(&(res), 0);
    res.dig[0] = max(n->dig[0], b->dig[0]) + 1;
    for(i = 1; i <= n->dig[0]; ++i)
        res.dig[i] = n->dig[i];
    for(i = 1; i <= b->dig[0]; ++i)
        res.dig[i] += b->dig[i];
    for(i = 1; i <= res.dig[0]; ++i){
        res.dig[i + 1] += res.dig[i] / base;
        res.dig[i] %= base;
    }
    while(res.dig[0] > 1 && !res.dig[res.dig[0]])
        --res.dig[0];
    return res;
}

biginteger sub(bigintegerP b, bigintegerP n){
    biginteger res;
    setValue(&(res), 0);
    res.dig[0] = max(n->dig[0], b->dig[0]) + 1;
    for(i = 1; i <= n->dig[0]; ++i)
        res.dig[i] -= n->dig[i];
    for(i = 1; i <= b->dig[0]; ++i)
        res.dig[i] += b->dig[i];
    for(i = 1; i <= res.dig[0]; ++i){
        res.dig[i + 1] += res.dig[i] / base;
        res.dig[i] %= base;
    }
    while(res.dig[0] > 1 && !res.dig[res.dig[0]])
        --res.dig[0];
    return res;
}

biginteger mul(bigintegerP b, bigintegerP n){
    biginteger res;
    setValue(&(res), 0);
    res.dig[0] = n->dig[0] + b->dig[0] + 1;
    for(i = 1; i <= n->dig[0]; ++i)
        for(j = 1; j <= b->dig[0]; ++j)
            res.dig[i + j - 1] += n->dig[i] * b->dig[j];
    for(i = 1; i <= res.dig[0]; ++i){
        res.dig[i + 1] += res.dig[i] / base;
        res.dig[i] %= base;
    }
    while(res.dig[0] > 1 && !res.dig[res.dig[0]])
        --res.dig[0];
    return res;
}

double _div(bigintegerP b, bigintegerP n){
    double a = 0.0, _b = 0.0;
    for(i = b->dig[0]; i; --i)
        a = a * base + b->dig[i];
    for(i = n->dig[0]; i; --i)
        _b = _b * base + n->dig[i];
    if(fabs(_b) <= 1e-8)
        errorDivideByZero = 1;
    return a / _b;
}

int operRat(char c){
    if(c == '+')
        return 1;
    if(c == '-')
        return 2;
    if(c == '*' || c == '/')
        return 3;
    if(c == '^')
        return 4;
    if(c == '!')
        return 5;
    return 11;
}

variable vadd(variableP a, variableP b){
    variable res;
    if(a->type == 1 && b->type == 1){
        res.type = 1;
        res.vl = a->vl + b->vl;
    }
    else if(a->type != 3 && b->type != 3){
        res.type = 2;
        res.vd = (double)(a->type == 1 ? a->vl : a->vd) + (b->type == 1 ? b->vl : b->vd);
    }
    else if(a->type == 1 || b->type == 1){
        res.type = 3;
        if(a->type == 1){
            setValue(&res.vb, a->vl);
            res.vb = add(&res.vb, &b->vb);
        }
        else{
            setValue(&res.vb, b->vl);
            res.vb = add(&res.vb, &a->vb);
        }
    }
    else if(a->type == 2 || b->type == 2){
        res.type = 2;
        if(a->type == 3)
            res.vd = convertToDouble(&a->vb) + b->vd;
        else
            res.vd = convertToDouble(&b->vb) + a->vd;
    }
    else{
        res.type = 3;
        res.vb = add(&a->vb, &b->vb);
    }
    return res;
}

variable vsub(variableP a, variableP b){
    variable res;
    if(a->type == 1 && b->type == 1){
        res.type = 1;
        res.vl = a->vl - b->vl;
    }
    else if(a->type != 3 && b->type != 3){
        res.type = 2;
        res.vd = (a->type == 1 ? a->vl : a->vd) - (b->type == 1 ? b->vl : b->vd);
    }
    else if(a->type == 1 || b->type == 1){
        res.type = 3;
        if(a->type == 1){
            setValue(&res.vb, a->vl);
            res.vb = sub(&res.vb, &b->vb);
        }
        else{
            setValue(&res.vb, b->vl);
            res.vb = sub(&a->vb, &res.vb);
        }
    }
    else if(a->type == 2 || b->type == 2){
        res.type = 2;
        if(a->type == 3)
            res.vd = convertToDouble(&a->vb) - b->vd;
        else
            res.vd = a->vd - convertToDouble(&b->vb);
    }
    else{
        res.type = 3;
        res.vb = sub(&a->vb, &b->vb);
    }
    return res;
}

variable vmul(variableP a, variableP b){
    variable res;
    biginteger tmp;
    if(a->type == 1 && b->type == 1){
        if(abs(a->vl * b->vl) <= base){
            res.type = 1;
            res.vl = a->vl * b->vl;
        }
        else{
            res.type = 3;
            setValue(&res.vb, a->vl);
            setValue(&tmp, b->vl);
            res.vb = mul(&res.vb, &tmp);
            return res;
        }
    }
    else if(a->type != 3 && b->type != 3){
        res.type = 2;
        res.vd = (a->type == 1 ? a->vl : a->vd) * (b->type == 1 ? b->vl : b->vd);
    }
    else if(a->type == 1 || b->type == 1){
        res.type = 3;
        if(a->type == 1){
            setValue(&res.vb, a->vl);
            res.vb = mul(&res.vb, &b->vb);
        }
        else{
            setValue(&res.vb, b->vl);
            res.vb = mul(&a->vb, &res.vb);
        }
    }
    else if(a->type == 2 || b->type == 2){
        res.type = 2;
        if(a->type == 3)
            res.vd = convertToDouble(&a->vb) * b->vd;
        else
            res.vd =  a->vd * convertToDouble(&b->vb);
    }
    else{
        res.type = 3;
        res.vb = mul(&a->vb, &b->vb);
    }
    return res;
}

variable vdiv(variableP a, variableP b){
    variable res;
    if(a->type == 1 && b->type == 1){
        if(b->vl == 0){
            errorDivideByZero = 1;
            return res;
        }
        if(a->vl % b->vl == 0){
            res.type = 1;
            res.vl = a->vl / b->vl;
        }
        else{
            res.type = 2;
            res.vd = (double)a->vl / b->vl;
        }
    }
    else if(a->type != 3 && b->type != 3){
        res.type = 2;
        if((b->type == 1 ? b->vl : b->vd) == 0)
            errorDivideByZero = 1;
        else
            res.vd = (a->type == 1 ? a->vl : a->vd) / (b->type == 1 ? b->vl : b->vd);
    }
    else if(a->type == 1 || b->type == 1){
        res.type = 2;
        if(a->type == 1){
            setValue(&res.vb, a->vl);
            if(b->vb.dig[0] == 1 && b->vb.dig[1] == 0)
                errorDivideByZero = 1;
            else
                res.vd = _div(&res.vb, &b->vb);
        }
        else{
            setValue(&res.vb, b->vl);
            if(b->vl == 0)
                errorDivideByZero = 1;
            else
                res.vd = _div(&a->vb, &res.vb);
        }
    }
    else if(a->type == 2 || b->type == 2){
        res.type = 2;
        if(a->type == 3){
            if(b->vd == 0)
                errorDivideByZero = 1;
            else
                res.vd = convertToDouble(&a->vb) / b->vd;
        }
        else{
            if(convertToDouble(&b->vb) == 0)
                errorDivideByZero = 1;
            else
                res.vd =  a->vd / convertToDouble(&b->vb);
        }
    }
    else{
        res.type = 2;
        if(b->vb.dig[0] == 1 && b->vb.dig[1] == 0)
            errorDivideByZero = 1;
        else
            res.vd = convertToDouble(&a->vb) / convertToDouble(&b->vb);
    }
    return res;
}

variable vpow(variableP a, variableP b){
    variable res;int tmp;int _mod;
    res.type = 2;
    if(a->type == 2 || b->type == 2)
        res.vd = pow((a->type == 1 ? a->vl : a->vd), (b->type == 1 ? b->vl : b->vd));
    else if(b->type == 1){
        if(b->vl == 0){
            res.type = res.vl = 1;
            return res;
        }
        tmp=b->vl;
        b->vl /= 2;
        res = vpow(a, b);
        res = vmul(&res, &res);
        if(tmp & 1)
            res = vmul(&res, a);
        return res;
    }
    else{
        if(b->vb.dig[0] == 1 && b->vb.dig[1] == 1){
            res.type = res.vl = 1;
            return res;
        }
        _mod = 0;
        for(i = b->vb.dig[0]; i; --i){
            b->vb.dig[i] += _mod * base;
            b->vb.dig[i] /= 2;
        }
        while(b->vb.dig[0] > 1 && b->vb.dig[b->vb.dig[0]] == 0)
            --b->vb.dig[0];
        res = vpow(a, b);
        res = vmul(&res, &res);
        if(_mod)
            res = vmul(&res, a);
        return res;

    }
    return res;
}

biginteger one;

variable fact(variableP x){
    variable res;
    double d;
    biginteger _mul;
    if(x->type == 2){
        res.type = 2;
        res.vd = 1;
        for(d = x->vd; d > 0; d -= 1)
            res.vd *= d;
    }
    else{
        res.type = 3;
        setValue(&res.vb, 1);
        one.dig[0] = one.dig[1] = 1;
        if(x->type == 1){
            setValue(&_mul, x->vl);
            while(_mul.dig[0] > 1 || _mul.dig[1] > 1){
                res.vb = mul(&res.vb, &_mul);
                _mul = sub(&_mul, &one);
            }
        }
    }
    return res;
}

double convertToA(double d){
    if(degreeType)
        return d;
    return d * acos(-1.0) / 180.;
}

double converToD(double a){
    if(degreeType)
        return a;
    return a * 180. / acos(-1.0);
}

variable calcExpr(int l, int r){
    variable res;
	int point;
    variable vl;variable vr;
    double tmp;
    int brac = 0, minRat = 10, pos = -1;
    for(i = l; i < r; ++i){
        if(expr[i] == '(')
            ++brac;
        else if(expr[i] == ')')
            --brac;
        else if(brac == 0){
            if(operRat(expr[i]) <= minRat)
                minRat = operRat(expr[i]), pos = i;
        }
    }
    if(pos != -1){
		vl = calcExpr(l, pos);
        if(expr[pos] == '!')
            return fact(&vl);
        vr = calcExpr(pos + 1, r);
        if(expr[pos] == '+')
            return vadd(&vl, &vr);
        if(expr[pos] == '-')
            return vsub(&vl, &vr);
        if(expr[pos] == '*')
            return vmul(&vl, &vr);
        if(expr[pos] == '/')
            return vdiv(&vl, &vr);
        if(expr[pos] == '^')
            return vpow(&vl, &vr);
    }

    if(l < r && expr[l] == '(' && expr[r - 1] == ')')
        return calcExpr(l + 1, r - 1);
    if(l < r && expr[l] == ' ')
        return calcExpr(l + 1, r);
    if(l < r && expr[r - 1] == ' ')
        return calcExpr(l, r - 1);

    if(l == r){
        res.type = 1;
        res.vl = 0;
        return res;
    }
    if(expr[l] == '('){
        errorExpr = 1;
        return res;
    }
    if((expr[l] >= '0' && expr[l] <= '9') || expr[l] == '-'){
        int dot = -1, sign = 0;
        if(expr[l] == '-')
            sign = 1, ++l;
        for(i = l; i < r; ++i)
            if(expr[i] == '.')
                dot = i;
        if(dot != -1){
            res.type = 2;
            res.vd = 0;
            pos = 0, point = 0;
            for(i = l; i < r; ++i)
                if(expr[i] == '.')
                    point = 1;
                else
                    res.vd = res.vd * 10.0 + expr[i] - '0', pos += point;
            while(pos)
                res.vd /= 10.0, --pos;
            if(sign)
                res.vd = -res.vd;
            return res;
        }
        if(r - l > 4){
            int pow = 1;
            res.type = 3;
            res.vb.dig[0] = 1;
			res.vb.dig[1] = 0;
            for(i = r - 1; i >= l; --i){
                res.vb.dig[res.vb.dig[0]] += pow * (expr[i] - '0');
                pow *= 10;
                if(pow == base && i > l){
                    pow = 1;
                    if(sign) 
                        res.vb.dig[res.vb.dig[0]] *= -1;
                    res.vb.dig[++res.vb.dig[0]] = 0;
                }
            }
            return res;
        }
        res.type = 1;
        res.vl = 0;
        for(i = l; i < r; ++i)
            res.vl = res.vl * 10 + expr[i] - '0';
        if(sign)
            res.vl *= -1;
        return res;
    }
    else if(expr[l] == 'P'){
        res.type = 2;
        res.vd = acos(-1.0);
        return res;
    }
    else if(expr[l] == 'E'){
        res.type = 2;
        res.vd = exp(1.0);
        return res;
    }
    else{
        int l_ = l;
        while(expr[l_] != '(' && l_ < r)
            ++l_;
        if(expr[l_] != '(')
            errorExpr = 1;
        res = calcExpr(l_, r);
        if(res.type == 1)
            tmp = res.vl;
        else if(res.type == 2)
            tmp = res.vd;
        else
            tmp = convertToDouble(&res.vb);
        switch(expr[l]){
            case 'c':
                if(expr[l + 1] == 'o' && expr[l + 2] == 's'){
                    if(expr[l + 3] == '(')
                        tmp = cos(convertToA(tmp));
                    else
                        tmp = converToD(cosh(convertToA(tmp)));
                }
                else if(expr[l + 1] == 't' && expr[l + 2] == 'g')
                    tmp = 1. / tan(convertToA(tmp));
                else
                    tmp = converToD(acos(-1.0) * 0.5 - tanh(convertToA(tmp)));
                break;
            case 's':
                if(expr[l + 1] == 'i' && expr[l + 2] == 'n'){
                    if(expr[l + 3] == '(')
                        tmp = sin(convertToA(tmp));
                    else
                        tmp = converToD(sinh(convertToA(tmp)));
                }
                else if(expr[l + 1] == 'q'){
                    if(tmp < 0)
                        errorExpr = 1;
                    tmp = sqrt(tmp);
                }
                else
                    tmp = (tmp < 0 ? -1 : tmp > 0);
                break;
            case 't':
                if(expr[l + 1] == 'g')
                    tmp = tan(convertToA(tmp));
                else
                    tmp = converToD(tanh(convertToA(tmp)));
                break;
            case 'a':
                tmp = fabs(tmp);
                break;
            case 'l':
                if(tmp <= 0)
                    errorExpr = 1;
                else
                    tmp = log(tmp);
                break;
        }
        res.type = 2;
        res.vd = tmp;
        return res;
    }
}

void outputAnswer(variable ans){
	int bitTrans,P;
    if(ans.type == 2 && fabs(ans.vd - floor(ans.vd)) < 1e-9){
        ans.type = 3;
        ans.vb.dig[0] = 0;
        bitTrans = 0, P = 1;
        if(ans.vd < 1)
            ans.vb.dig[0] = 1, ans.vb.dig[1] = 0;
        while(ans.vd >= 1){
            if(!bitTrans)
                ans.vb.dig[++ans.vb.dig[0]] = 0, P = 1;
            ans.vb.dig[ans.vb.dig[0]] = ans.vb.dig[ans.vb.dig[0]] + (int)floor(ans.vd - floor(ans.vd / 10) * 10) * P;
            bitTrans = (bitTrans + 1) % 4;
            P *= 10;
            ans.vd /= 10;
        }
    }
    if(ans.type == 1)
        printf("%d", (int)(ans.vl));
    else if(ans.type == 2)
        printf(outputFmt, ans.vd);
    else{
        printf("%d", (int)(ans.vb.dig[ans.vb.dig[0]]));
        for(i = ans.vb.dig[0] - 1; i; --i)
            printf("%04d", (int)(ans.vb.dig[i]));
    }
}

void read(int *n){
    int tmp = -1;
    char c = getchar();
    while(c != '\n'){
        if(c >= '0' && c <= '9')
            tmp = max(0, tmp) * 10 + c - '0';
        c = getchar();
    }
    if(tmp != -1)
        *n = (int)tmp;
}

void init(){
    printf("表达式计算器\n请输入计算精度(默认保留3位小数):");
    read(&eps);  // 缺省为3
    printf("请设置角度表示方式.角度为0,弧度为1(默认为弧度):");
    read(&degreeType);  // 缺省为1 
    sprintf(outputFmt, "%%.%dlf", eps);
}

void readin(char s[]){
    int len = 0;
    while((s[len++] = getchar()) != '\n');
    s[--len] = '\0';
}
void _setmode(int e,int d)
{
	degreeType=d;
	eps=e;
	sprintf(outputFmt, "%%.%dlf", eps);
}

int _calc(char *ex,char *ans)
{
	variable res;
    strcpy(expr,ex);
    errorDivideByZero=0;
    errorExpr=0;
    res=calcExpr(0,strlen(expr));
    if(errorExpr)
	{
		strcpy(ans,"错误：表达式语法错误");
        return 1;
    }
    if(errorDivideByZero)
	{
		strcpy(ans,"错误：除数为 0");
        return 2;
    }
    _outputAnswer(res,ans);
    return 0;
}

void _outputAnswer(variable ans,char *szans){
	int bitTrans,P;
	char *tmp;
    if(ans.type == 2 && fabs(ans.vd - floor(ans.vd)) < 1e-9){
        ans.type = 3;
        ans.vb.dig[0] = 0;
        bitTrans = 0, P = 1;
        if(ans.vd < 1)
            ans.vb.dig[0] = 1, ans.vb.dig[1] = 0;
        while(ans.vd >= 1){
            if(!bitTrans)
                ans.vb.dig[++ans.vb.dig[0]] = 0, P = 1;
            ans.vb.dig[ans.vb.dig[0]] = ans.vb.dig[ans.vb.dig[0]] + (int)floor(ans.vd - floor(ans.vd / 10) * 10) * P;
            bitTrans = (bitTrans + 1) % 4;
            P *= 10;
            ans.vd /= 10;
        }
    }
    tmp=(char*)malloc(2560000*sizeof(char));
    if(tmp)
    {
	    szans[0]=0;
	    if(ans.type == 1)
	        sprintf(szans,"%d", (int)(ans.vl));
	    else if(ans.type == 2)
	        sprintf(szans,outputFmt, ans.vd);
	    else{
	        sprintf(szans,"%d", (int)(ans.vb.dig[ans.vb.dig[0]]));
	        for(i = ans.vb.dig[0] - 1; i; --i)
	        {
	            sprintf(tmp,"%04d", ans.vb.dig[i]>=0?(int)(ans.vb.dig[i]):(int)(-ans.vb.dig[i]));
	            strcat(szans,tmp);
	        }
	    }
	    free(tmp);
	}
	else
	{
		MessageBox(NULL,"_outputAnswer 内存不足","错误",MB_OK|MB_ICONSTOP);
		sprintf(szans,"_outputAnswer 内存不足");
	}
}

