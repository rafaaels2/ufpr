require 'rubygems'
require 'active_record'

ActiveRecord::Base.establish_connection :adapter => "sqlite3",
                                        :database => "Tabelas.sqlite3"

ActiveRecord::Base.connection.create_table :carros do |t|
    t.integer :crv, limit: 4
    t.string :nome
    t.string :marca
end