require 'active_record'

class String
    def only_letters?
        !!match(/[[:alpha:]]+$/)
    end

    def only_numbers?
        !!match(/^\d+$/)
    end
end

ActiveRecord::Base.establish_connection :adapter => "sqlite3",
                                        :database => "Tabelas.sqlite3"

class Pessoa < ActiveRecord::Base;
    validate :rg_tam_4, :idade_maxTam_3, :somente_caracteres, :somente_numeros

    def rg_tam_4
        if rg.length != 4
            errors.add(:rg, "# RG deve que ter 4 algarismos")
        end
    end

    def idade_maxTam_3
        if idade.length > 3
            errors.add(:idade, "# Idade deve ter no máximo 3 algarismos")
        end
    end

    def somente_caracteres
        if !nome.only_letters?
            errors.add(:nome, "# Nome deve ter somente caracteres")
        end

        if !cidade.only_letters?
            errors.add(:nome, "# Cidade deve ter somente caracteres")
        end
    end

    def somente_numeros
        if !rg.only_numbers?
            errors.add(:rg, "# Rg deve ter somente numeros")
        end

        if !idade.only_numbers?
            errors.add(:idade, "# Idade deve ter somente caracteres")
        end
    end
end